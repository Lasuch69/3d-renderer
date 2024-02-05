import os, fnmatch
import shader_gen

def find(pattern, path):
    result = []
    for root, dirs, files in os.walk(path):
        for name in files:
            if fnmatch.fnmatch(name, pattern):
                result.append(os.path.join(root, name))
    return result

shaders = find('*.glsl', 'src/rendering/shaders')
shader_gen.build_headers(shaders)

include = [
    'thirdparty/vma',
    'thirdparty/stb',
    'thirdparty/tiny_obj',
    'thirdparty/imgui',
    'thirdparty/imgui/backends',
    'thirdparty/glslang',
    'SDL2/include'
]

env = Environment(CC='gcc', CCFLAGS='-O2', COMPILATIONDB_USE_ABSPATH=True, CPPPATH = include)
env.Tool('compilation_db')
env.CompilationDatabase()

files = find('*.cpp', '.')
env.Program('renderer', files, LIBS = ['SDL2', 'vulkan'])
