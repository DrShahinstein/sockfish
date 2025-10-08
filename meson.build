project('sockfish', 'c', version : '0.1.0', default_options : ['warning_level=3', 'c_std=c11'])

sdl3_dep       = dependency('sdl3')
sdl3_image_dep = dependency('sdl3-image')
sdl3_ttf_dep   = dependency('sdl3-ttf')

inc      = include_directories('include', 'src/engine/')
sdl3_inc = include_directories(sdl3_dep.get_variable(pkgconfig: 'includedir'))

sources = files(
  'src/main.c',
  'src/board/board.c',
  'src/board/board_event.c',
  'src/board/board_render.c',
  'src/board/special_moves.c',
  'src/board/parse_pgn_move.c',
  'src/ui/ui.c',
  'src/ui/ui_render.c',
  'src/ui/ui_event.c',
  'src/ui/ui_helpers.c',
  'src/ui/cursor.c',
  'src/engine/engine.c',
  'src/engine/bitboard_maker.c',
  'src/engine/position_hasher.c',
  'src/engine/sockfish/bitboard.c',
  'src/engine/sockfish/search.c',
  'src/engine/sockfish/movegen.c',
  'src/engine/sockfish/move_helper.c',
)

executable('sockfish',
  sources,
  include_directories : [inc, sdl3_inc],
  dependencies : [sdl3_dep, sdl3_image_dep, sdl3_ttf_dep],
  install : true
)
