"""Compare two Sockfish UCI engines on a small fixed-depth suite."""

from __future__ import annotations

import argparse
import os
import queue
import statistics
import subprocess
import sys
import tarfile
import tempfile
import threading
import time
from collections import Counter
from dataclasses import dataclass
from pathlib import Path
from typing import Callable, Optional


ROOT              = Path(__file__).resolve().parents[1]
DEFAULT_POSITIONS = Path(__file__).with_name("fundamentals.fen")


class BenchError(RuntimeError):
  pass


@dataclass(frozen=True)
class Position:
  name: str
  fen: str


@dataclass(frozen=True)
class Result:
  nodes: int
  time_ms: int
  score_type: str
  score: int
  bestmove: str
  pv: str


class UciEngine:
  def __init__(self, label: str, executable: Path, timeout: float):
    self.label   = label
    self.timeout = timeout
    self.output: queue.Queue[Optional[str]] = queue.Queue()
    self.process = subprocess.Popen(
      [str(executable)],
      stdin=subprocess.PIPE,
      stdout=subprocess.PIPE,
      stderr=None,
      text=True,
      bufsize=1,
    )

    if self.process.stdin is None or self.process.stdout is None:
      raise BenchError(f"{label}: could not open engine pipes")

    threading.Thread(target=self._read_stdout, daemon=True).start()

  def _read_stdout(self) -> None:
    assert self.process.stdout is not None
    for line in self.process.stdout:
      self.output.put(line.rstrip("\r\n"))
    self.output.put(None)

  def send(self, command: str) -> None:
    if self.process.poll() is not None:
      raise BenchError(f"{self.label}: engine exited with {self.process.returncode}")

    assert self.process.stdin is not None
    self.process.stdin.write(command + "\n")
    self.process.stdin.flush()

  def read_until(self, done: Callable[[str], bool]) -> list[str]:
    deadline = time.monotonic() + self.timeout
    lines: list[str] = []

    while True:
      remaining = deadline - time.monotonic()
      if remaining <= 0:
        raise BenchError(f"{self.label}: search timed out")

      try:
        line = self.output.get(timeout=remaining)
      except queue.Empty as exc:
        raise BenchError(f"{self.label}: search timed out") from exc

      if line is None:
        raise BenchError(f"{self.label}: engine closed its output")

      lines.append(line)
      if done(line):
        return lines

  def ready(self) -> None:
    self.send("isready")
    self.read_until(lambda line: line == "readyok")

  def initialize(self, hash_mb: int) -> None:
    self.send("uci")
    self.read_until(lambda line: line == "uciok")
    self.send(f"setoption name Hash value {hash_mb}")
    self.send("setoption name Threads value 1")
    self.ready()

  def search(self, position: Position, depth: int) -> Result:
    self.send("ucinewgame")
    self.ready()
    self.send(f"position fen {position.fen}")
    self.send(f"go depth {depth}")
    lines = self.read_until(lambda line: line.startswith("bestmove "))

    info = [line for line in lines if line.startswith("info ")]
    best = [line for line in lines if line.startswith("bestmove ")]
    if not info or not best:
      raise BenchError(f"{self.label}: incomplete output for {position.name}")

    parsed = parse_info(info[-1])
    if parsed["depth"] != depth:
      raise BenchError(
        f"{self.label}: {position.name} reached depth {parsed['depth']}, expected {depth}"
      )

    return Result(
      nodes=parsed["nodes"],
      time_ms=max(parsed["time_ms"], 1),
      score_type=parsed["score_type"],
      score=parsed["score"],
      bestmove=best[-1].split()[1],
      pv=parsed["pv"],
    )

  def close(self) -> None:
    if self.process.poll() is not None:
      return

    try:
      self.send("quit")
      self.process.wait(timeout=5)
    except (BenchError, subprocess.TimeoutExpired):
      self.process.kill()
      self.process.wait(timeout=5)


def info_value(tokens: list[str], key: str) -> str:
  try:
    return tokens[tokens.index(key) + 1]
  except (ValueError, IndexError) as exc:
    raise BenchError(f"cannot parse UCI info field {key!r}") from exc


def parse_info(line: str) -> dict[str, object]:
  tokens = line.split()
  try:
    score_index = tokens.index("score")
    pv_index    = tokens.index("pv")
    return {
      "depth": int(info_value(tokens, "depth")),
      "nodes": int(info_value(tokens, "nodes")),
      "time_ms": int(info_value(tokens, "time")),
      "score_type": tokens[score_index + 1],
      "score": int(tokens[score_index + 2]),
      "pv": " ".join(tokens[pv_index + 1:]),
    }
  except (ValueError, IndexError) as exc:
    raise BenchError(f"cannot parse UCI info line: {line}") from exc


def load_positions(path: Path) -> list[Position]:
  try:
    lines = path.read_text(encoding="utf-8").splitlines()
  except OSError as exc:
    raise BenchError(f"cannot read {path}: {exc}") from exc

  positions: list[Position] = []
  names: set[str] = set()

  for line_number, raw in enumerate(lines, start=1):
    line = raw.strip()
    if not line or line.startswith("#"):
      continue
    if "|" not in line:
      raise BenchError(f"{path}:{line_number}: expected 'name | FEN'")

    name, fen = (part.strip() for part in line.split("|", 1))
    if not name or len(fen.split()) != 6:
      raise BenchError(f"{path}:{line_number}: invalid name or FEN")
    if name in names:
      raise BenchError(f"{path}:{line_number}: duplicate name {name!r}")

    names.add(name)
    positions.append(Position(name, fen))

  if not positions:
    raise BenchError(f"{path}: no benchmark positions")

  return positions


def run_command(command: list[str]) -> None:
  result = subprocess.run(
    command,
    cwd=ROOT,
    text=True,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
  )
  if result.returncode != 0:
    raise BenchError(f"command failed: {' '.join(command)}\n{result.stdout}")


def build_ref(ref: str, directory: Path) -> Path:
  source  = directory / "source"
  build   = directory / "build"
  archive = directory / "source.tar"
  source.mkdir(parents=True)

  print(f"[build] {ref}", file=sys.stderr)
  run_command(["git", "archive", "--format=tar", f"--output={archive}", ref])
  with tarfile.open(archive) as tar:
    tar.extractall(source)

  run_command(["meson", "setup", str(build), str(source), "--buildtype=release"])
  run_command(["meson", "compile", "-C", str(build), "sockfish-uci"])

  suffix = ".exe" if os.name == "nt" else ""
  executable = build / f"sockfish-uci{suffix}"
  if not executable.is_file():
    raise BenchError(f"{ref}: engine was not built at {executable}")
  return executable


def existing_executable(value: str) -> Path:
  path = Path(value).expanduser().resolve()
  if not path.is_file():
    raise BenchError(f"engine does not exist: {path}")
  if os.name != "nt" and not os.access(path, os.X_OK):
    raise BenchError(f"engine is not executable: {path}")
  return path


def median_result(runs: list[Result]) -> Result:
  nodes   = round(statistics.median(run.nodes for run in runs))
  time_ms = round(statistics.median(run.time_ms for run in runs))
  score_type, score = Counter((run.score_type, run.score) for run in runs).most_common(1)[0][0]
  bestmove = Counter(run.bestmove for run in runs).most_common(1)[0][0]
  representative = next(
    run for run in runs
    if run.bestmove == bestmove     and
       run.score_type == score_type and
       run.score == score
  )

  return Result(
    nodes=nodes,
    time_ms=max(time_ms, 1),
    score_type=score_type,
    score=score,
    bestmove=bestmove,
    pv=representative.pv,
  )


def benchmark(
  specs: list[tuple[str, Path]],
  positions: list[Position],
  depth: int,
  repeat: int,
  hash_mb: int,
  timeout: float,
) -> dict[str, dict[str, Result]]:
  engines: list[UciEngine] = []
  runs: dict[str, dict[str, list[Result]]] = {
    label: {position.name: [] for position in positions}
    for label, _ in specs
  }

  try:
    for label, executable in specs:
      engine = UciEngine(label, executable, timeout)
      engines.append(engine)
      engine.initialize(hash_mb)
      engine.search(positions[0], min(depth, 4))  # warm up

    for pass_index in range(repeat):
      for position_index, position in enumerate(positions):
        order = engines if (pass_index + position_index) % 2 == 0 else reversed(engines)
        for engine in order:
          print(
            f"[{engine.label}] pass {pass_index + 1}/{repeat}: {position.name}",
            file=sys.stderr,
          )
          runs[engine.label][position.name].append(engine.search(position, depth))
  finally:
    for engine in engines:
      engine.close()

  return {
    label: {
      position.name: median_result(runs[label][position.name])
      for position in positions
    }
    for label, _ in specs
  }


def percent(baseline: int, candidate: int) -> float:
  return 0.0 if baseline == 0 else (candidate / baseline - 1.0) * 100.0


def report(
  specs: list[tuple[str, Path]],
  positions: list[Position],
  results: dict[str, dict[str, Result]],
  depth: int,
  repeat: int,
  hash_mb: int,
) -> None:
  baseline_label  = specs[0][0]
  candidate_label = specs[1][0]
  baseline        = results[baseline_label]
  candidate       = results[candidate_label]

  print(
    f"\nFixed-depth benchmark: depth={depth}, repeat={repeat}, "
    f"threads=1, hash={hash_mb} MB"
  )
  print(f"Baseline:  {baseline_label}")
  print(f"Candidate: {candidate_label}\n")

  header = (
    f"{'position':<21} {'base nodes':>12} {'cand nodes':>12} {'nodes Δ':>9} "
    f"{'base ms':>9} {'cand ms':>9} {'time Δ':>9}"
  )
  print(header)
  print("-" * len(header))

  for position in positions:
    base = baseline[position.name]
    cand = candidate[position.name]
    print(
      f"{position.name:<21} {base.nodes:>12,} {cand.nodes:>12,} "
      f"{percent(base.nodes, cand.nodes):>+8.1f}% "
      f"{base.time_ms:>9,} {cand.time_ms:>9,} "
      f"{percent(base.time_ms, cand.time_ms):>+8.1f}%"
    )

  base_nodes = sum(result.nodes for result in baseline.values())
  cand_nodes = sum(result.nodes for result in candidate.values())
  base_time  = sum(result.time_ms for result in baseline.values())
  cand_time  = sum(result.time_ms for result in candidate.values())
  base_nps   = (base_nodes * 1000) // base_time
  cand_nps   = (cand_nodes * 1000) // cand_time
  agreements = sum(
    baseline[position.name].bestmove == candidate[position.name].bestmove
    for position in positions
  )

  print("\nSummary")
  print(f"  nodes: {base_nodes:,} -> {cand_nodes:,} ({percent(base_nodes, cand_nodes):+.1f}%)")
  print(f"  time:  {base_time:,} ms -> {cand_time:,} ms ({percent(base_time, cand_time):+.1f}%)")
  print(f"  NPS:   {base_nps:,} -> {cand_nps:,} ({percent(base_nps, cand_nps):+.1f}%)")
  print(f"  best-move agreement: {agreements}/{len(positions)}")

  changes = []
  for position in positions:
    base = baseline[position.name]
    cand = candidate[position.name]
    if (
      base.bestmove != cand.bestmove or
      base.score_type != cand.score_type or
      base.score != cand.score
    ):
      changes.append((position.name, base, cand))

  if changes:
    print("\nChanged search results")
    for name, base, cand in changes:
      print(
        f"  {name}: {base.bestmove} ({base.score_type} {base.score}) -> "
        f"{cand.bestmove} ({cand.score_type} {cand.score})"
      )


def parse_args() -> argparse.Namespace:
  parser = argparse.ArgumentParser(description=__doc__)
  source = parser.add_mutually_exclusive_group(required=True)
  source.add_argument("--refs", nargs=2, metavar=("BASELINE_REF", "CANDIDATE_REF"))
  source.add_argument("--executables", nargs=2, metavar=("BASELINE_EXE", "CANDIDATE_EXE"))
  parser.add_argument("--depth", type=int, default=11)
  parser.add_argument("--repeat", type=int, default=1)
  parser.add_argument("--hash-mb", type=int, default=64)
  parser.add_argument("--timeout", type=float, default=60.0)
  parser.add_argument("--positions", type=Path, default=DEFAULT_POSITIONS)
  return parser.parse_args()


def main() -> int:
  args = parse_args()

  try:
    if min(args.depth, args.repeat, args.hash_mb) <= 0 or args.timeout <= 0:
      raise BenchError("depth, repeat, hash size, and timeout must be positive")

    positions = load_positions(args.positions)

    with tempfile.TemporaryDirectory(prefix="sockfish-bench-") as temporary:
      temp = Path(temporary)
      if args.refs:
        baseline_ref, candidate_ref = args.refs
        specs = [
          (f"baseline:{baseline_ref}",   build_ref(baseline_ref,  temp / "baseline")),
          (f"candidate:{candidate_ref}", build_ref(candidate_ref, temp / "candidate")),
        ]
      else:
        baseline_exe, candidate_exe = args.executables
        specs = [
          ("baseline",  existing_executable(baseline_exe)),
          ("candidate", existing_executable(candidate_exe)),
        ]

      results = benchmark(
        specs,
        positions,
        args.depth,
        args.repeat,
        args.hash_mb,
        args.timeout,
      )
      report(specs, positions, results, args.depth, args.repeat, args.hash_mb)

    return 0

  except (BenchError, OSError) as exc:
    print(f"error: {exc}", file=sys.stderr)
    return 1


if __name__ == "__main__":
  raise SystemExit(main())
