cmake_minimum_required(VERSION 3.5)
project(3d-renderer C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
set(LIBRARIES "glfw;Vulkan::Vulkan")

file(GLOB_RECURSE SOURCES src/*.cpp)

# Dear ImGui
file(GLOB_RECURSE IMGUI_SOURCES thirdparty/imgui/*.cpp
     thirdparty/imgui/backends/*.cpp)

# glslang
set(GLSL_SOURCES
    thirdparty/glslang/OGLCompilersDLL/InitializeDll.cpp
    thirdparty/glslang/SPIRV/InReadableOrder.cpp
    thirdparty/glslang/SPIRV/SpvTools.cpp
    thirdparty/glslang/SPIRV/GlslangToSpv.cpp
    thirdparty/glslang/SPIRV/Logger.cpp
    thirdparty/glslang/SPIRV/SPVRemapper.cpp
    thirdparty/glslang/SPIRV/SpvBuilder.cpp
    thirdparty/glslang/SPIRV/SpvPostProcess.cpp
    thirdparty/glslang/SPIRV/disassemble.cpp
    thirdparty/glslang/SPIRV/doc.cpp
    thirdparty/glslang/glslang/GenericCodeGen/CodeGen.cpp
    thirdparty/glslang/glslang/GenericCodeGen/Link.cpp
    thirdparty/glslang/glslang/MachineIndependent/InfoSink.cpp
    thirdparty/glslang/glslang/MachineIndependent/IntermTraverse.cpp
    thirdparty/glslang/glslang/MachineIndependent/RemoveTree.cpp
    thirdparty/glslang/glslang/MachineIndependent/parseConst.cpp
    thirdparty/glslang/glslang/MachineIndependent/preprocessor/PpAtom.cpp
    thirdparty/glslang/glslang/MachineIndependent/preprocessor/PpContext.cpp
    thirdparty/glslang/glslang/MachineIndependent/preprocessor/Pp.cpp
    thirdparty/glslang/glslang/MachineIndependent/preprocessor/PpScanner.cpp
    thirdparty/glslang/glslang/MachineIndependent/preprocessor/PpTokens.cpp
    thirdparty/glslang/glslang/MachineIndependent/Constant.cpp
    thirdparty/glslang/glslang/MachineIndependent/Initialize.cpp
    thirdparty/glslang/glslang/MachineIndependent/Intermediate.cpp
    thirdparty/glslang/glslang/MachineIndependent/ParseContextBase.cpp
    thirdparty/glslang/glslang/MachineIndependent/ParseHelper.cpp
    thirdparty/glslang/glslang/MachineIndependent/PoolAlloc.cpp
    thirdparty/glslang/glslang/MachineIndependent/Scan.cpp
    thirdparty/glslang/glslang/MachineIndependent/ShaderLang.cpp
    thirdparty/glslang/glslang/MachineIndependent/SpirvIntrinsics.cpp
    thirdparty/glslang/glslang/MachineIndependent/SymbolTable.cpp
    thirdparty/glslang/glslang/MachineIndependent/Versions.cpp
    thirdparty/glslang/glslang/MachineIndependent/attribute.cpp
    thirdparty/glslang/glslang/MachineIndependent/glslang_tab.cpp
    thirdparty/glslang/glslang/MachineIndependent/intermOut.cpp
    thirdparty/glslang/glslang/MachineIndependent/iomapper.cpp
    thirdparty/glslang/glslang/MachineIndependent/limits.cpp
    thirdparty/glslang/glslang/MachineIndependent/linkValidate.cpp
    thirdparty/glslang/glslang/MachineIndependent/propagateNoContraction.cpp
    thirdparty/glslang/glslang/MachineIndependent/reflection.cpp
    thirdparty/glslang/glslang/OSDependent/Unix/ossource.cpp
    # thirdparty/glslang/glslang/OSDependent/Web/glslang.js.cpp
    # thirdparty/glslang/glslang/OSDependent/Windows/main.cpp
    # thirdparty/glslang/glslang/OSDependent/Windows/ossource.cpp
    thirdparty/glslang/glslang/ResourceLimits/ResourceLimits.cpp)

set(DEPENDENCIES thirdparty/vma/vk_mem_alloc.cpp thirdparty/stb/stb_image.cpp
                 thirdparty/tiny_obj/tiny_obj_loader.cpp)

include_directories(
  thirdparty/vma thirdparty/stb thirdparty/tiny_obj thirdparty/imgui
  thirdparty/imgui/backends thirdparty/glslang)

add_executable(renderer ${SOURCES} ${DEPENDENCIES} ${IMGUI_SOURCES}
                        ${GLSL_SOURCES})
target_link_libraries(renderer ${LIBRARIES})
