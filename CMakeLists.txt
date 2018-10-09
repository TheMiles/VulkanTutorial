cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
cmake_policy(VERSION 3.7)

set(NAME VulkanTutorial)

project(${NAME})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)
find_package(glm REQUIRED)

find_package(Vulkan REQUIRED)

include_directories(${GLFW_INCLUDE_DIRS} Vulkan::Vulkan external/stb external/tinyobjloader)
set(SOURCES triangleMain.cpp)

add_executable(vulkan ${SOURCES})
target_link_libraries(vulkan ${GLFW_LIBRARIES} Vulkan::Vulkan glm)

file(GLOB shader_files  RELATIVE ${PROJECT_SOURCE_DIR} "shaders/*.vert" "shaders/*.frag")
string(REPLACE ".vert" "_vert.spv" shader_files "${shader_files}")
string(REPLACE ".frag" "_frag.spv" shader_files "${shader_files}")
foreach(shader_file ${shader_files})
    configure_file("${shader_file}" "${shader_file}" COPYONLY)
endforeach(shader_file)

file(GLOB texture_files RELATIVE ${PROJECT_SOURCE_DIR} "textures/*")
foreach(texture_file ${texture_files})
    configure_file("${texture_file}" "${texture_file}" COPYONLY)
endforeach(texture_file)
