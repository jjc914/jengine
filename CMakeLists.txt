cmake_minimum_required(VERSION 3.5)

project(editor LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

add_subdirectory(lib/wulkan)
add_subdirectory(lib/simple_ecs)

# tinyobjloader
add_library(tinyobjloader STATIC vendor/tinyobjloader/tiny_obj_loader.cc)
target_include_directories(tinyobjloader PUBLIC vendor/tinyobjloader)

# dear imgui
find_package(Vulkan REQUIRED)
include_directories(${Vulkan_INCLUDE_DIRS})

file(GLOB IMGUI_SOURCES
    vendor/imgui/*.cpp
    vendor/imgui/backends/imgui_impl_glfw.cpp
    vendor/imgui/backends/imgui_impl_vulkan.cpp
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC ${CMAKE_SOURCE_DIR}/vendor/glfw/include)
target_include_directories(imgui PUBLIC vendor/imgui vendor/imgui/backends)

# glm
include_directories(vendor/glm)

# engine library
add_library(engine STATIC)
target_compile_definitions(engine PUBLIC WLK_ENABLE_VALIDATION_LAYERS)
target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(engine PRIVATE
    engine/core/debug/assert.hpp
    engine/core/debug/logger.hpp

    engine/core/window/window.hpp

    engine/core/graphics/descriptor_set_layout.hpp
    engine/core/graphics/descriptor_types.hpp
    engine/core/graphics/device.hpp
    engine/core/graphics/frame_graph.hpp
    engine/core/graphics/image_types.hpp
    engine/core/graphics/instance.hpp
    engine/core/graphics/material.hpp
    engine/core/graphics/mesh_buffer.hpp
    engine/core/graphics/pipeline.hpp
    engine/core/graphics/render_target.hpp
    engine/core/graphics/shader.hpp
    engine/core/graphics/vertex_types.hpp

    engine/import/mesh.hpp   engine/import/mesh.cpp

    engine/drivers/vulkan/convert_vulkan.hpp
    engine/drivers/vulkan/vulkan_descriptor_set_layout.hpp  engine/drivers/vulkan/vulkan_descriptor_set_layout.cpp
    engine/drivers/vulkan/vulkan_device.hpp                 engine/drivers/vulkan/vulkan_device.cpp
    engine/drivers/vulkan/vulkan_instance.hpp               engine/drivers/vulkan/vulkan_instance.cpp
    engine/drivers/vulkan/vulkan_material.hpp               engine/drivers/vulkan/vulkan_material.cpp
    engine/drivers/vulkan/vulkan_mesh_buffer.hpp            engine/drivers/vulkan/vulkan_mesh_buffer.cpp
    engine/drivers/vulkan/vulkan_pipeline.hpp               engine/drivers/vulkan/vulkan_pipeline.cpp
    engine/drivers/vulkan/vulkan_render_target.hpp
    engine/drivers/vulkan/vulkan_shader.hpp                 engine/drivers/vulkan/vulkan_shader.cpp
    engine/drivers/vulkan/vulkan_texture_render_target.hpp  engine/drivers/vulkan/vulkan_texture_render_target.cpp
    engine/drivers/vulkan/vulkan_viewport.hpp               engine/drivers/vulkan/vulkan_viewport.cpp

    engine/drivers/glfw/glfw_window.hpp       engine/drivers/glfw/glfw_window.cpp
)
target_link_libraries(engine PUBLIC wulkan simple_ecs tinyobjloader)

add_executable(editor 
    editor/main.cpp
    editor/app.hpp  editor/app.cpp
    editor/gui/imgui_layer.hpp  editor/gui/imgui_layer.cpp
)

target_compile_definitions(editor PUBLIC WLK_ENABLE_VALIDATION_LAYERS)
target_link_libraries(editor PRIVATE imgui engine)
target_include_directories(editor PUBLIC ${CMAKE_SOURCE_DIR})

# ----------------------------
# Resource and Shader Management
# ----------------------------

find_package(Vulkan REQUIRED)
get_filename_component(VULKAN_PATH ${Vulkan_INCLUDE_DIRS} DIRECTORY)
set(GLSL_VALIDATOR "${VULKAN_PATH}/bin/glslangValidator")

# --- Copy General Resources ---
set(RES_SRC_DIR "${CMAKE_SOURCE_DIR}/res")
set(RES_DST_DIR "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/res")

add_custom_command(
    TARGET editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${RES_DST_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${RES_SRC_DIR}" "${RES_DST_DIR}"
    COMMENT "Copying resources to runtime res folder"
)

# --- Compile Shaders ---
set(SHADER_SRC_DIR "${CMAKE_SOURCE_DIR}/editor/shaders")
set(SHADER_BUILD_DIR "${CMAKE_BINARY_DIR}/shaders")
set(SHADER_RUNTIME_DIR "${CMAKE_BINARY_DIR}/bin/$<CONFIG>/shaders")

file(GLOB_RECURSE GLSL_SOURCES
    "${SHADER_SRC_DIR}/*.vert"
    "${SHADER_SRC_DIR}/*.frag"
)

set(SPIRV_OUTPUTS "")
foreach(GLSL_FILE ${GLSL_SOURCES})
    get_filename_component(FILE_NAME ${GLSL_FILE} NAME)
    set(SPIRV_FILE "${SHADER_BUILD_DIR}/${FILE_NAME}.spv")

    add_custom_command(
        OUTPUT ${SPIRV_FILE}
        COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_BUILD_DIR}"
        COMMAND "${GLSL_VALIDATOR}" -V "${GLSL_FILE}" -o "${SPIRV_FILE}"
        DEPENDS ${GLSL_FILE}
        COMMENT "Compiling shader: ${FILE_NAME}"
        BYPRODUCTS ${SPIRV_FILE}
    )

    list(APPEND SPIRV_OUTPUTS ${SPIRV_FILE})
endforeach()

add_custom_target(compile_shaders DEPENDS ${SPIRV_OUTPUTS})
add_dependencies(editor compile_shaders)

# --- Copy Compiled Shaders to Executable Folder ---
add_custom_command(
    TARGET editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${SHADER_RUNTIME_DIR}"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${SHADER_BUILD_DIR}" "${SHADER_RUNTIME_DIR}"
    COMMENT "Copying SPIR-V shaders to runtime folder"
)