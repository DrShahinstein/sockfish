project('sockfish', 'c', version : '0.1.0',
  default_options : [
    'c_std=c11',
    'warning_level=3',
  ]
)

sdl3_dep       = dependency('sdl3')
sdl3_image_dep = dependency('sdl3-image')
sdl3_ttf_dep   = dependency('sdl3-ttf')
thread_dep     = dependency('threads')

inc      = include_directories('include', 'src/engine/')
sdl3_inc = include_directories(sdl3_dep.get_variable(pkgconfig: 'includedir'))

common_sources = files(
  'src/app.c',
  'src/board/board.c',
  'src/board/board_event.c',
  'src/board/board_render.c',
  'src/board/special_moves.c',
  'src/board/annotations.c',
  'src/board/parse_pgn_move.c',
  'src/ui/ui.c',
  'src/ui/ui_render.c',
  'src/ui/ui_event.c',
  'src/ui/ui_helpers.c',
  'src/ui/cache.c',
  'src/ui/cursor.c',
  'src/ui/info.c',
  'src/engine/engine.c',
  'src/engine/sockfish/sockfish.c',
  'src/engine/sockfish/bitboard.c',
  'src/engine/sockfish/search.c',
  'src/engine/sockfish/movegen.c',
  'src/engine/sockfish/move_helper.c',
  'src/engine/sockfish/evaluation.c',
  'src/engine/sockfish/zobrist.c',
  'src/engine/sockfish/transposition_table.c',
  'src/engine/sockfish/platform.c',
  'src/engine/sockfish/uci.c',
  'src/engine/sockfish/config.c',
  'src/engine/sockfish/thread.c',
)

sockfish_lib = static_library('sockfish',
  common_sources,
  include_directories : [inc, sdl3_inc],
  dependencies : [sdl3_dep, sdl3_image_dep, sdl3_ttf_dep, thread_dep]
)

is_debugging = get_option('buildtype') == 'debug'

executable('sockfish-sdl',
  sources: 'src/main_sdl.c',
  link_with : sockfish_lib,
  include_directories : [inc, sdl3_inc],
  win_subsystem : is_debugging ? 'console' : 'windows',
  install : true,
)

executable('sockfish-uci',
  sources: 'src/main_uci.c',
  link_with : sockfish_lib,
  include_directories : [inc, sdl3_inc],
  win_subsystem : 'console',
  install : true,
)

