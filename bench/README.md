# Search Benchmark

This is a small fixed-depth benchmark for comparing two Sockfish revisions. It
is intended to catch search-performance regressions quickly.

## Compare Git refs

The easiest way to compare committed revisions is:

```bash
python3 bench/search_bench.py --refs origin/main dev
```

The script exports both refs into a temporary directory, builds release-mode
UCI engines, runs the benchmark, and removes the temporary builds afterward.
Only committed content is included.

Use `HEAD` to benchmark the current commit:

```bash
python3 bench/search_bench.py --refs origin/main HEAD
```

## Compare existing executables

This avoids rebuilding when you already have two release binaries:

```bash
python3 bench/search_bench.py \
  --executables /path/to/main/sockfish-uci /path/to/dev/sockfish-uci
```

For a less noisy measurement, increase the depth and repeat count:

```bash
python3 bench/search_bench.py \
  --refs origin/main HEAD \
  --depth 15 \
  --repeat 3 | tee /tmp/sockfish-bench.txt
```

## Complex positions

`complex-positions.fen` contains all 24 positions from the Bratko-Kopec Test.
Run that suite with:

```bash
python3 bench/search_bench.py             \
  ...                                     \
  --positions bench/complex-positions.fen \
  ...                                     \
  ...                                     \
```

## Reading the result

- `nodes` compares search-tree size at the same depth. Fewer nodes can indicate
  better pruning, but it does not prove that the engine is stronger.
- `time` is the engine-reported search time. Lower is better for the same work.
- `NPS` is useful for detecting raw speed regressions.
- Changed scores and best moves are listed for inspection, not marked as
  automatic failures.
