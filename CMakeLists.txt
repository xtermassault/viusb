cmake_minimum_required(VERSION 3.16)

project(USBViewer)

include(GNUInstallDirs)

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

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cuteusb.desktop.in
        ${CMAKE_CURRENT_BINARY_DIR}/cuteusb.desktop
        @ONLY
)

install(TARGETS usb_viewer
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/cuteusb.desktop
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/icon-512.png
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/512x512/apps
        RENAME cuteusb.png
)
