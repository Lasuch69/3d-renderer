cmake_minimum_required(VERSION 3.5)
project(3d-renderer C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Vulkan REQUIRED)
set(LIBRARIES "glfw;Vulkan::Vulkan")

file(GLOB_RECURSE SOURCES src/*.cpp)

# Dear ImGui
set(IMGUI_DIR thirdparty/imgui)
file(GLOB_RECURSE IMGUI_SOURCES ${IMGUI_DIR}/*.cpp ${IMGUI_DIR}/backends/*.cpp)

# Header Libs
set(VMA_DIR thirdparty/vma)
set(STB_DIR thirdparty/stb)
set(TINY_DIR thirdparty/tiny_obj)

set(DEPENDENCIES ${VMA_DIR}/vk_mem_alloc.cpp ${STB_DIR}/stb_image.cpp
                 ${TINY_DIR}/tiny_obj_loader.cpp ${IMGUI_SOURCES})

include_directories(${VMA_DIR} ${STB_DIR} ${TINY_DIR} ${IMGUI_DIR}
                    ${IMGUI_DIR}/backends)

add_executable(renderer ${SOURCES} ${DEPENDENCIES})
target_link_libraries(renderer ${LIBRARIES})
