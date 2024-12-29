cmake_minimum_required(VERSION 3.5)

project(USBViewer)

set(CMAKE_CXX_STANDARD 20)

find_package(Qt5 REQUIRED COMPONENTS Widgets Core)

add_executable(usb_viewer
        main.cpp
        usb_sysfs.cpp
)

target_link_libraries(usb_viewer Qt5::Widgets Qt5::Core)
