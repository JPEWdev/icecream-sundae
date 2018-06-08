project('icecream-sundae', 'cpp',
        version: '1.0.0',
        meson_version: '>= 0.31.0',
        default_options: [
            'buildtype=debugoptimized',
            'cpp_std=c++14',
            'warning_level=3',
            'b_ndebug=true',
            ])

icecc = dependency('icecc')
glib = dependency('glib-2.0')
ncurses = dependency('ncursesw')

cxx = meson.get_compiler('cpp')

libdl = cxx.find_library('dl')

deps = [icecc, glib, ncurses, libdl]

code = '''
#include <icecc/comm.h>

int main(int argc, char **argv)
{
    std::string net = "net";
    std::string sched = "sched";
    auto p = new DiscoverSched(net, 0, sched);
    (void)p;
    return 0;
}
'''
if not cxx.links(code, dependencies: deps, name: 'libicecc links dynamically')
    icecc = dependency('icecc', static: true)
    liblzo2 = cxx.find_library('lzo2')
    libcapng = dependency('libcap-ng')
    deps += [libcapng, liblzo2]
endif

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

if not cxx.links(code, dependencies: deps, name: 'libicecc C++11 compatability')
    add_global_arguments('-D_GLIBCXX_USE_CXX11_ABI=0', language: 'cpp')
endif

conf_data = configuration_data()
conf_data.set('version', meson.project_version())
configure_file(input: 'config.h.in', output: 'config.h', configuration: conf_data)

icecream_sundae = executable('icecream-sundae',
    ['src/main.cpp', 'src/draw.cpp', 'src/scheduler.cpp', 'src/simulator.cpp'],
    include_directories: incdir,
    dependencies: deps,
    install : true
    )

# A pretty dumb test right now. Simply run the simulator with a known key for a
# fixed number of iterations and see if anything breaks
test('Random simulator test', icecream_sundae, is_parallel: false,
    args: ['--simulate', '--sim-seed=123456', '--sim-cycles=10000', '--sim-speed=1'],
    env: ['ASAN_OPTIONS=detect_leaks=1:leak_check_at_exit=true:verbosity=1', 'TERM=dumb'],
    )
