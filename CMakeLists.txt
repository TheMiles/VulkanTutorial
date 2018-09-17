cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
cmake_policy(VERSION 3.7)

set(NAME VulkanTutorial)

project(${NAME})

find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

find_package(Vulkan REQUIRED)

include_directories(${GLFW_INCLUDE_DIRS} Vulkan::Vulkan)
set(SOURCES triangleMain.cpp)

add_executable(vulkan ${SOURCES})
target_link_libraries(vulkan ${GLFW_LIBRARIES} Vulkan::Vulkan)
