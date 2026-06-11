cmake_minimum_required(VERSION 3.27)
project(FluidSimulation)

set(CMAKE_CXX_STANDARD 17)

find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)

add_library(glad STATIC glad.c)
target_include_directories(glad PUBLIC ${CMAKE_SOURCE_DIR}/includes)

add_library(imgui
        includes/imgui/imgui.cpp
        includes/imgui/imgui_draw.cpp
        includes/imgui/imgui_widgets.cpp
        includes/imgui/imgui_tables.cpp
        includes/imgui/backends/imgui_impl_glfw.cpp
        includes/imgui/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
        includes/imgui
        includes/imgui/backends
)

add_executable(FluidSimulation main.cpp
        Physics.cpp
        Physics.h
        Render.cpp
        Render.h)

target_link_libraries(FluidSimulation
        glad
        glfw
        OpenGL::GL
        imgui
)

add_definitions(-DGL_SILENCE_DEPRECATION)