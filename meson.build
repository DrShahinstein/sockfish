project('sockfish', 'c', version : '0.1.0', default_options : ['warning_level=3', 'c_std=c11'])

sdl3_dep       = dependency('sdl3')
sdl3_image_dep = dependency('sdl3-image')
sdl3_ttf_dep   = dependency('sdl3-ttf')

inc      = include_directories('include')
sdl3_inc = include_directories(sdl3_dep.get_variable(pkgconfig: 'includedir'))

sources = files(
  'src/main.c',
  'src/game.c',
  'src/board.c',
  'src/event.c',
  'src/ui.c',
  'src/cursor.c',
)

executable('sockfish',
  sources,
  include_directories : [inc, sdl3_inc],
  dependencies : [sdl3_dep, sdl3_image_dep, sdl3_ttf_dep],
  install : true
)
