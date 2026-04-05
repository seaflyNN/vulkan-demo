cmake_minimum_required(VERSION 3.25)
project(vulkan-hello-world C CXX)

set(CMAKE_CXX_STANDARD 23)

find_package(Vulkan REQUIRED)
find_program(GLSLC_EXECUTABLE
    NAMES glslc glslc.exe
    HINTS
    "$ENV{VULKAN_SDK}/Bin"
    PATHS
    "C:/VulkanSDK/1.4.341.1/Bin"
    REQUIRED
)

set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shader)
set(SHADER_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/shader)

set(VERT_SHADER_SOURCE ${SHADER_SOURCE_DIR}/shader.vert)
set(FRAG_SHADER_SOURCE ${SHADER_SOURCE_DIR}/shader.frag)
set(VERT_SHADER_SPV ${SHADER_BINARY_DIR}/vert.spv)
set(FRAG_SHADER_SPV ${SHADER_BINARY_DIR}/frag.spv)

add_custom_command(
    OUTPUT ${VERT_SHADER_SPV}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_BINARY_DIR}
    COMMAND ${GLSLC_EXECUTABLE} ${VERT_SHADER_SOURCE} -o ${VERT_SHADER_SPV}
    DEPENDS ${VERT_SHADER_SOURCE}
    COMMENT "Compiling vertex shader"
    VERBATIM
)

add_custom_command(
    OUTPUT ${FRAG_SHADER_SPV}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_BINARY_DIR}
    COMMAND ${GLSLC_EXECUTABLE} ${FRAG_SHADER_SOURCE} -o ${FRAG_SHADER_SPV}
    DEPENDS ${FRAG_SHADER_SOURCE}
    COMMENT "Compiling fragment shader"
    VERBATIM
)

add_custom_target(shaders DEPENDS ${VERT_SHADER_SPV} ${FRAG_SHADER_SPV})

add_subdirectory(3rd/SDL-release-3.4.2 sdl3)

add_executable(vulkan-hello-world main.cpp)
add_dependencies(vulkan-hello-world shaders)
target_include_directories(vulkan-hello-world PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(vulkan-hello-world PRIVATE HF_SHADER_DIR="${SHADER_BINARY_DIR}")

message("${Vulkan_INCLUDE_DIRS}")
message("${Vulkan_LIBRARIES}")

target_include_directories(vulkan-hello-world PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(vulkan-hello-world PRIVATE
    ${Vulkan_LIBRARIES}
    SDL3::SDL3
)
if(MSVC)
    target_compile_options(vulkan-hello-world PRIVATE /utf-8)
endif()
