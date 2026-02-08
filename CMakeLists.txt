cmake_minimum_required(VERSION 3.16)
project(DOSBOSS_3D VERSION 0.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find packages
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

# ImGui sources
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_impl_sdl2.cpp
    ${IMGUI_DIR}/imgui_impl_opengl3.cpp
)

# Application sources
set(APP_SOURCES
    src/main.cpp
    src/core/types.cpp
    src/core/project.cpp
    src/engine/renderer.cpp
    src/engine/geometry.cpp
    src/engine/camera.cpp
    src/engine/gizmo.cpp
    src/editor/editor.cpp
    src/editor/viewport.cpp
    src/editor/properties.cpp
    src/editor/event_editor.cpp
    src/editor/timeline.cpp
    src/runtime/runtime.cpp
)

add_executable(dosboss_3d ${APP_SOURCES} ${IMGUI_SOURCES})

target_include_directories(dosboss_3d PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party/imgui
    ${CMAKE_SOURCE_DIR}/third_party/json
    ${SDL2_INCLUDE_DIRS}
)

target_link_libraries(dosboss_3d PRIVATE
    ${SDL2_LIBRARIES}
    OpenGL::GL
    GLEW::GLEW
)

target_compile_options(dosboss_3d PRIVATE ${SDL2_CFLAGS_OTHER})

# Copy assets if they exist
if(EXISTS ${CMAKE_SOURCE_DIR}/assets)
    file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR})
endif()
