#!/usr/bin/env python

target = 'renderer'
sources = Glob('src/*.cpp') + Glob('thirdparty/*/*.cpp')
libs = ['glfw', 'vulkan', 'dl', 'pthread',
        'X11', 'Xxf86vm', 'Xrandr', 'Xi']
include = ['thirdparty/']

env = Environment(COMPILATIONDB_USE_ABSPATH=True)
env.Tool('compilation_db')
env.CompilationDatabase()
env.Program(target=target,
            source=sources,
            LIBS=libs,
            CPPPATH=include)
