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
    ${SHARED_SRC}
)

# GUI executable
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
