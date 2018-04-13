project('icecream-sundae', 'cpp')

version = '1.0.0'

icecc = dependency('icecc')
glib = dependency('glib-2.0')
ncurses = dependency('ncursesw')

incdir = include_directories('src')

conf_data = configuration_data()
conf_data.set('version', version)
configure_file(input: 'config.h.in', output: 'config.h', configuration: conf_data)

executable('icecream-sundae', ['src/main.cpp', 'src/draw.cpp',
        'src/scheduler.cpp', 'src/simulator.cpp'],
    include_directories: incdir,
    dependencies: [icecc, glib, ncurses],
    install : true
    )
