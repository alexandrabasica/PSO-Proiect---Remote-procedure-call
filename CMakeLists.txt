cmake_minimum_required(VERSION 3.16)
project(PSO_Proiect)

set(CMAKE_CXX_STANDARD 17)

# Include path comun
include_directories(shared)

# Cod comun
set(SHARED_SRC
    shared/Client.cpp
    shared/Database.cpp
    shared/tinyxml2.cpp
)

# Server executable
add_executable(pso_server
    server/main.cpp
    server/Server.cpp
    server/Tracer.cpp
    ${SHARED_SRC}
)

# GUI executable
set(OPENGL_opengl_LIBRARY /usr/lib/x86_64-linux-gnu/libGL.so.1)
set(OPENGL_glx_LIBRARY /usr/lib/x86_64-linux-gnu/libGL.so.1)
set(OPENGL_INCLUDE_DIR /usr/include)
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

if(NOT TARGET OpenGL::GL)
    add_library(OpenGL::GL UNKNOWN IMPORTED)
    set_target_properties(OpenGL::GL PROPERTIES
        IMPORTED_LOCATION "${OPENGL_opengl_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${OPENGL_INCLUDE_DIR}"
    )
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(pso_gui
    gui/gui_main.cpp
    gui/mainwindow.cpp
    ${SHARED_SRC}
)

target_link_libraries(pso_gui Qt6::Widgets)
