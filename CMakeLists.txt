cmake_minimum_required(VERSION 3.5)

project(ViUSB)

set(CMAKE_CXX_STANDARD 20)

find_package(Qt5 REQUIRED COMPONENTS Widgets Core)

set(SOURCES
        main.cpp
        usb_libusb.cpp
        usb_sysfs.cpp
)

add_executable(viusb ${SOURCES})

target_link_libraries(viusb Qt5::Widgets Qt5::Core usb-1.0)
