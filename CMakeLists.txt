cmake_minimum_required(VERSION 3.16)
project(json_parser VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(BUILD_GUI "Build the Qt 6 window (needs Qt 6)" ON)

if(MSVC)
  # Sources and messages are in Bulgarian (UTF-8).
  add_compile_options(/utf-8 /W4)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ---------- JSON library: parser, writer, paths, commands (no Qt) ----------
add_library(json_core STATIC
  src/core/value.cpp
  src/core/parser.cpp
  src/core/writer.cpp
  src/core/path.cpp
  src/core/document.cpp
  src/core/shell.cpp
)
target_include_directories(json_core PUBLIC src/core)

# ---------- Console ----------
add_executable(json_cli src/cli/main.cpp)
target_link_libraries(json_cli PRIVATE json_core)

# ---------- Window ----------
if(BUILD_GUI)
  find_package(Qt6 6.2 COMPONENTS Core Gui Widgets)
  if(Qt6_FOUND)
    # Qt links the AGL framework on macOS, but recent macOS SDKs no longer ship
    # it. The app does not use OpenGL, so drop it.
    if(APPLE AND TARGET WrapOpenGL::WrapOpenGL)
      get_target_property(_gl_libs WrapOpenGL::WrapOpenGL INTERFACE_LINK_LIBRARIES)
      if(_gl_libs)
        list(FILTER _gl_libs EXCLUDE REGEX "AGL")
        set_target_properties(WrapOpenGL::WrapOpenGL PROPERTIES INTERFACE_LINK_LIBRARIES "${_gl_libs}")
      endif()
    endif()

    set(CMAKE_AUTOMOC ON)
    add_library(json_ui STATIC
      src/gui/mainwindow.cpp
      src/gui/codeeditor.cpp
      src/gui/highlighter.cpp
      src/gui/jsontree.cpp
    )
    target_include_directories(json_ui PUBLIC src/gui)
    target_link_libraries(json_ui PUBLIC json_core Qt6::Widgets)

    add_executable(json_gui WIN32 MACOSX_BUNDLE src/gui/main.cpp)
    target_link_libraries(json_gui PRIVATE json_ui)
  else()
    message(WARNING "Qt 6 не е намерен: компилира се само конзолната версия (json_cli).")
  endif()
endif()

# ---------- Tests ----------
include(CTest)
if(BUILD_TESTING)
  add_executable(json_tests tests/test_json.cpp)
  target_link_libraries(json_tests PRIVATE json_core)
  target_compile_definitions(json_tests PRIVATE EXAMPLES_DIR="${CMAKE_CURRENT_SOURCE_DIR}/examples")
  add_test(NAME json_tests COMMAND json_tests)
endif()
