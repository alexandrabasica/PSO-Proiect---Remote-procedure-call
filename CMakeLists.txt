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

find_package(OpenGL REQUIRED)
include_directories(${OPENGL_INCLUDE_DIR})

if(NOT TARGET OpenGL::GL)
    if(OPENGL_gl_LIBRARY)
        add_library(OpenGL::GL UNKNOWN IMPORTED)
        set_target_properties(OpenGL::GL PROPERTIES
            IMPORTED_LOCATION "${OPENGL_gl_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${OPENGL_INCLUDE_DIR}"
        )
    else()
         message(WARNING "OpenGL::GL target not found and could not be created manually.")
    endif()
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
