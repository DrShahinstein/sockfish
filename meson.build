project('sockfish', 'c', version : '0.1.0', default_options : ['warning_level=3', 'c_std=c11'])

sdl3_dep       = dependency('sdl3')
sdl3_image_dep = dependency('sdl3_image')

inc = include_directories('include')

sources = files(
  'src/main.c',
)

executable('sockfish',
  sources,
  include_directories : inc,
  dependencies : [sdl3_dep, sdl3_image_dep],
  install : true
)
