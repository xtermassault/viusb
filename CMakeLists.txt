cmake_minimum_required(VERSION 3.5)

project(ViUSB)

set(CMAKE_CXX_STANDARD 11)

find_package(Qt5Widgets REQUIRED)

add_executable(viusb main.cpp)

target_link_libraries(viusb Qt5::Widgets)
