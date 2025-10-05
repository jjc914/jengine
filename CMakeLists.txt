cmake_minimum_required(VERSION 3.5)

project(jengine LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Central output directory for all executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_subdirectory(lib/wulkan)
add_subdirectory(lib/simple_ecs)

add_executable(jengine 
    src/main.cpp
    src/core/app.hpp                               src/core/app.cpp
    src/core/window/window.hpp
    src/core/window/glfw_window.hpp                src/core/window/glfw_window.cpp
    src/core/graphics/instance.hpp
    src/core/graphics/device.hpp
    src/core/graphics/renderer.hpp
    src/core/graphics/pipeline.hpp
    src/core/graphics/mesh_buffer.hpp
    src/core/graphics/material.hpp
    src/core/graphics/vertex_layout.hpp
    src/core/graphics/vulkan/vk_instance.hpp       src/core/graphics/vulkan/vk_instance.cpp
    src/core/graphics/vulkan/vk_device.hpp         src/core/graphics/vulkan/vk_device.cpp
    src/core/graphics/vulkan/vk_renderer.hpp       src/core/graphics/vulkan/vk_renderer.cpp
    src/core/graphics/vulkan/vk_pipeline.hpp       src/core/graphics/vulkan/vk_pipeline.cpp
    src/core/graphics/vulkan/vk_mesh_buffer.hpp    src/core/graphics/vulkan/vk_mesh_buffer.cpp
    src/core/graphics/vulkan/vk_material.hpp       src/core/graphics/vulkan/vk_material.cpp
    src/utils/extent.hpp
)

target_compile_definitions(jengine PUBLIC WLK_ENABLE_VALIDATION_LAYERS)
target_link_libraries(jengine PUBLIC wulkan simple_ecs)

# ----------------
# shader compilation
# ----------------
find_package(Vulkan REQUIRED)

get_filename_component(VULKAN_PATH ${Vulkan_INCLUDE_DIRS} DIRECTORY)
set(GLSL_VALIDATOR ${VULKAN_PATH}/bin/glslangValidator)

file(GLOB_RECURSE GLSL_SOURCE_FILES "shaders/*.vert" "shaders/*.frag")

foreach(GLSL ${GLSL_SOURCE_FILES})
    get_filename_component(FILE_NAME ${GLSL} NAME)
    set(SPIRV "${PROJECT_BINARY_DIR}/shaders/${FILE_NAME}.spv")
    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${PROJECT_BINARY_DIR}/shaders/"
        COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
        DEPENDS ${GLSL}
    )
    list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach()

add_custom_target(jengineshaders DEPENDS ${SPIRV_BINARY_FILES})
add_dependencies(jengine jengineshaders)

add_custom_command(TARGET jengine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:jengine>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_BINARY_DIR}/shaders"
        "$<TARGET_FILE_DIR:jengine>/shaders"
)
