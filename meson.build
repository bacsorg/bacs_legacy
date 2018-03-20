project('bacs_legacy', ['c', 'cpp'],
        version : '0.1.0',
        license : 'apache2',
        meson_version : '>=0.46',
        default_options : [
            'cpp_std=c++17',
            'warning_level=3',
        ])

srcs = [
    'src/lib/helper.cpp',
    'src/lib/test.cpp',
    'src/lib/db.cpp',
    'src/lib/run.cpp',
    'src/lib/tempfiles.cpp',
    'src/lib/log.cpp',
    'src/lib/util.cpp',
    'src/lib/common.cpp',
    'src/lib/problem.cpp',
    'src/lib/config.cpp',
    'src/lib/submit.cpp',
    'src/bin/bacsd.cpp',
]

util_install = join_paths(get_option('libdir'), 'bacs', 'legacy')
executable('checkdef', 'src/bin/checkdef.cpp',
    install : true,
    install_dir : util_install,
)
executable('limit_run', 'src/bin/limit_run.cpp',
    dependencies : meson.get_compiler('cpp').find_library('rt'),
    install : true,
    install_dir : util_install,
)
executable('java_compile', 'src/bin/java_compile.c',
    install : true,
    install_dir : util_install,
)
executable('wipe', 'src/bin/wipe.c',
    install : true,
    install_dir : util_install,
)
executable('java_run', 'src/bin/java_run.c',
    install : true,
    install_dir : util_install,
)

executable('bacsd', srcs,
    dependencies : [
        dependency('mariadb'),
        dependency('boost', modules : ['system', 'filesystem']),
        dependency('bunsan_pm_compatibility'),
    ],
    include_directories : include_directories('include'),
    install : true,
)

install_data('src/bin/py3_compile.py',
    install_dir : util_install,
    install_mode : 'rwxr-xr-x',
)
install_data('init_env.sh',
    install_dir : get_option('bindir'),
    install_mode : 'rwxr-xr-x',
    rename : 'bacs-legacy-makeenv',
)
