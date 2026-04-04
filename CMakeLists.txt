cmake_minimum_required(VERSION 3.25)
project(vulkan-hello-world C CXX)

set(CMAKE_CXX_STANDARD 23)

find_package(Vulkan REQUIRED)

add_subdirectory(3rd/SDL-release-3.4.2 sdl3)

add_executable(vulkan-hello-world main.cpp)
target_include_directories(vulkan-hello-world PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

message("${Vulkan_INCLUDE_DIRS}")
message("${Vulkan_LIBRARIES}")

target_include_directories(vulkan-hello-world PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(vulkan-hello-world PRIVATE
    ${Vulkan_LIBRARIES}
    SDL3::SDL3
)
