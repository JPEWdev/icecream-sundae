project('icecream-sundae', 'cpp')

version = '1.0.0'

icecc = dependency('icecc')
glib = dependency('glib-2.0')
ncurses = dependency('ncursesw')

cxx = meson.get_compiler('cpp')
libdl = cxx.find_library('dl')

incdir = include_directories('src')

code  = '''
#include <icecc/comm.h>
#include <memory>

int main(int argc, char **argv) {
    std::string net = "net";
    std::string sched = "sched";
    auto p = std::make_unique<DiscoverSched>(net, 0, sched);
    (void)p;
    return 0;
}
'''

if not cxx.links(code, dependencies: [icecc, libdl], name: 'libicecc C++11 compatability')
    add_global_arguments('-D_GLIBCXX_USE_CXX11_ABI=0', language: 'cpp')
endif

conf_data = configuration_data()
conf_data.set('version', version)
configure_file(input: 'config.h.in', output: 'config.h', configuration: conf_data)

executable('icecream-sundae', ['src/main.cpp', 'src/draw.cpp',
        'src/scheduler.cpp', 'src/simulator.cpp'],
    include_directories: incdir,
    dependencies: [icecc, glib, ncurses, libdl],
    install : true
    )
