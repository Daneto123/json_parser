cmake_minimum_required(VERSION 3.16)
project(StorageApp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB SOURCES "backend/src/*.cpp")

add_library(backend_lib
    ${SOURCES}
)

target_include_directories(backend_lib PUBLIC backend/include)

# ---------- GUI ----------
find_package(Qt5 REQUIRED COMPONENTS Core Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

add_executable(json_gui
    JsonUI/main.cpp
    JsonUI/mainwindow.cpp
    JsonUI/mainwindow.h
)

target_link_libraries(json_gui
    PRIVATE
    Qt5::Core
    Qt5::Widgets
    backend_lib
)

target_include_directories(json_gui
    PRIVATE JsonUI
)