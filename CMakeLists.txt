cmake_minimum_required(VERSION 3.5)

project(USBViewer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Core)

add_executable(usb_viewer
        main.cpp
        usb_sysfs.cpp
        main_window.ui
)

target_link_libraries(usb_viewer Qt6::Widgets Qt6::Core)
