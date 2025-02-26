cmake_minimum_required(VERSION 3.19)
project(P2Pal LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_PREFIX_PATH "C:/Qt/6.5.0/mingw_64")

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

qt_standard_project_setup()


qt_add_executable(P2Pal
    WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp
    mainwindow.h
    networking.cpp
    networking.h
    vectorclock.cpp
    vectorclock.h
)

target_link_libraries(P2Pal
    PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Network
)

include(GNUInstallDirs)

install(TARGETS P2Pal
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET P2Pal
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
