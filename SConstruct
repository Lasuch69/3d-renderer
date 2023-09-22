#!/usr/bin/env python

target = 'renderer'
sources = Glob('src/*.cpp')
libs = ['glfw', 'vulkan', 'dl', 'pthread',
        'X11', 'Xxf86vm', 'Xrandr', 'Xi']
includes = ['include/']

env = Environment(COMPILATIONDB_USE_ABSPATH=True)
env.Tool('compilation_db')
env.CompilationDatabase()
env.Program(target=target,
            source=sources,
            LIBS=libs,
            CPPPATH=includes)