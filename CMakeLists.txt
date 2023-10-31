cmake_minimum_required(VERSION 3.5)

# Definitions
set(PROJECT_NAME jengine)
set(TEST_NAME test)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/bin")
set(CMAKE_BUILD_TYPE Debug)

project(${PROJECT_NAME})
set(CMAKE_CXX_STANDARD 17)

## Vendor dependencies
# TODO: change this to not be my directory
set(VULKAN_PATH "/Users/joshuachasnov/VulkanSDK/1.3.261.1/macOS")
set(VULKAN_INCLUDE_DIRS ${VULKAN_PATH}/include)
find_package(Vulkan REQUIRED FATAL_ERROR)
set(LIB_DEPENDENCIES ${LIB_DEPENDENCIES} Vulkan::Vulkan shaderc_shared)
link_directories(${VULKAN_PATH}/lib)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/glfw)

# TODO: check to see if other things are building that are unnecessary
set(GLM_TEST_ENABLE OFF CACHE BOOL "" FORCE)
add_subdirectory(vendor/glm)
set(LIB_DEPENDENCIES ${LIB_DEPENDENCIES} glm)

find_package(OpenGL REQUIRED)
set(LIB_DEPENDENCIES ${LIB_DEPENDENCIES} OpenGL::GL glfw)

## Self dependencies (libraries)
add_subdirectory(lib/data-structure-lib)
set(LIB_DEPENDENCIES ${LIB_DEPENDENCIES} dslib)
add_subdirectory(lib/util-lib)
set(LIB_DEPENDENCIES ${LIB_DEPENDENCIES} utilib)

## Self dependencies (source)
set(SOURCE_FILES ${SOURCE_FILES}
    src/ecs/ecs.hpp
    src/engine/engine.hpp src/engine/engine.cpp
    src/gui/gui.hpp src/gui/gui.cpp
    src/editor/editor.hpp src/editor/editor.cpp
    src/editor/screen.hpp src/editor/screen.cpp
    src/editor/vlk_allocator.hpp src/editor/vlk_allocator.cpp
)

set(SOURCE_FILES ${SOURCE_FILES} src/main.cpp)
add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC src)

target_link_libraries(${PROJECT_NAME} ${LIB_DEPENDENCIES})
target_link_libraries(${PROJECT_NAME} ${SRC_DEPENDENCIES})

add_subdirectory(test)

target_include_directories(${PROJECT_NAME} PUBLIC ${VULKAN_INCLUDE_DIRS})

## Compile shaders (https://gist.github.com/evilactually/a0d191701cb48f157b05be7f74d79396)
set(GLSL_VALIDATOR ${VULKAN_PATH}/bin/glslangValidator)
file(GLOB_RECURSE GLSL_SOURCE_FILES
    src/editor/shaders/*.vert src/editor/shaders/*.frag
)

foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
        COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL})
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)

add_custom_target(
    Shaders
    DEPENDS ${SPIRV_BINARY_FILES}
)

add_dependencies(${PROJECT_NAME} Shaders)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_BINARY_DIR}/shaders"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
)
