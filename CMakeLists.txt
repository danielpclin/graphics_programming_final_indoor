cmake_minimum_required(VERSION 3.12)
project(graphics_programming)

include_directories(include)
link_directories(lib)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include/imgui)
add_library(IMGUI STATIC)

target_sources(IMGUI
    PRIVATE
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_impl_opengl3.cpp
    ${IMGUI_DIR}/imgui_impl_glfw.cpp
)

#add_compile_definitions(NDEBUG)
add_executable(graphics_programming src/main.cpp src/Camera.cpp src/Model.cpp src/Shader.cpp src/Texture.cpp src/TinyObjectModel.cpp)

set(LIBS opengl32.lib glew32.lib glfw3dll.lib IMGUI assimp.lib)
target_link_libraries(graphics_programming ${LIBS})

#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

# copy graphics_programming dll to bin dir
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/dll"
        $<TARGET_FILE_DIR:${PROJECT_NAME}>)

add_custom_target(
    resource_target ALL
)

add_dependencies(${PROJECT_NAME} resource_target)

# copy shader
add_custom_command(TARGET resource_target POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/shader"
        "${PROJECT_BINARY_DIR}/bin/shader")

# copy assets
add_custom_command(TARGET resource_target POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/assets"
        "${PROJECT_BINARY_DIR}/bin/assets")

# set build dir to project dir
#set_target_properties(graphics_programming
#    PROPERTIES
#    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}"
#)

# Only works in unix
## symlink shader folder
#add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E create_symlink
#        "${PROJECT_SOURCE_DIR}/shader"
#        $<TARGET_FILE_DIR:${PROJECT_NAME}>/shader)
#
## symlink assets folder
#add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E create_symlink
#        "${PROJECT_SOURCE_DIR}/assets"
#        $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets)
