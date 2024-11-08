cmake_minimum_required(VERSION 3.5)

# Definitions
set(PROJECT_NAME jengine)
set(TEST_NAME test_jengine)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build/bin")
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON) 

project(${PROJECT_NAME})

## Vendor dependencies
find_package(Vulkan REQUIRED)
set(VULKAN_PATH ${Vulkan_INCLUDE_DIRS}/../)
set(VULKAN_INCLUDE_DIRS ${VULKAN_PATH}/include)
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

add_subdirectory(vendor/VulkanMemoryAllocator)
set(LIB_DEPENDENCIES ${LIB_DEPENDENCIES} GPUOpen::VulkanMemoryAllocator)

# -------------- IMGUI static library creation`
#
set(IMGUI_PATH ${CMAKE_CURRENT_LIST_DIR}/vendor/imgui)
file(GLOB IMGUI_GLOB
    ${IMGUI_PATH}/imgui.h
    ${IMGUI_PATH}/imgui.cpp
    ${IMGUI_PATH}/imconfig.h
    ${IMGUI_PATH}/imgui_demo.cpp
    ${IMGUI_PATH}/imgui_draw.cpp
    ${IMGUI_PATH}/imgui_internal.h
    ${IMGUI_PATH}/imstb_rectpack.h
    ${IMGUI_PATH}/imstb_textedit.h
    ${IMGUI_PATH}/imstb_truetype.h
    ${IMGUI_PATH}/imgui_tables.cpp
    ${IMGUI_PATH}/imgui_widgets.cpp

    # specific bindings...
    ${IMGUI_PATH}/backends/imgui_impl_glfw.h
    ${IMGUI_PATH}/backends/imgui_impl_glfw.cpp
    ${IMGUI_PATH}/backends/imgui_impl_vulkan.h
    ${IMGUI_PATH}/backends/imgui_impl_vulkan.cpp)
add_library(imgui STATIC ${IMGUI_GLOB})
target_include_directories(imgui PUBLIC ${IMGUI_PATH})
target_include_directories(imgui PUBLIC ${IMGUI_PATH}/backends)
target_link_libraries(imgui PRIVATE glfw)
target_link_libraries(imgui PRIVATE Vulkan::Vulkan)
set(LIB_DEPENDENCIES ${LIB_DEPENDENCIES} imgui)
# ----------------------------------------------------------------------

## Sources
set(SOURCE_FILES ${SOURCE_FILES}
    src/core/api/GraphicsApi.hpp
    src/core/api/InputApi.hpp
    src/core/api/WindowApi.hpp
    src/core/ecs/Entity.hpp
    src/core/ecs/Component.hpp        src/core/ecs/Component.cpp
    src/core/ecs/System.hpp
    src/core/ecs/Archetype.hpp        src/core/ecs/Archetype.cpp
    src/core/ecs/EcsManager.hpp       src/core/ecs/EcsManager.cpp
    src/utils/SparseVector.hpp
    src/utils/StructureLog.hpp
    src/renderer/vulkan/VulkanApi.hpp src/renderer/vulkan/VulkanApi.cpp
    src/window/glfw/GlfwApi.hpp       src/window/glfw/GlfwApi.cpp
    src/editor_ui/ImGuiLayer.hpp      src/editor_ui/ImGuiLayer.cpp
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
    src/renderer/vulkan/shaders/*.vert src/renderer/vulkan/shaders/*.frag
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
