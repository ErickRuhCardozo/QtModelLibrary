cmake_minimum_required(VERSION 3.14)

project(QtModelLibrary LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Sql)

add_library(QtModelLibrary SHARED
  QtModelLibrary_global.hpp
  Model.cpp
  Model.hpp
)

target_link_libraries(QtModelLibrary PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::Sql)
target_compile_definitions(QtModelLibrary PRIVATE QTMODELLIBRARY_LIBRARY)
