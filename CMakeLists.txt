# SPDX-License-Identifier: GPL-3.0-or-later
cmake_minimum_required(VERSION 3.21)

project(shuangpin-overlay
    VERSION 0.1.2
    DESCRIPTION "A floating Shuangpin key chart that follows what you type"
    HOMEPAGE_URL "https://github.com/zhenban/shuangpin-overlay"
    LANGUAGES CXX)

set(APP_ID "io.github.zhenban.ShuangpinOverlay")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(GNUInstallDirs)
include(CTest)

option(WITH_LAYER_SHELL "Use the layer-shell protocol through LayerShellQt when available" ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Widgets DBus LinguistTools)

set(HAVE_LAYER_SHELL OFF)
if(WITH_LAYER_SHELL)
    find_package(LayerShellQt QUIET)
    if(LayerShellQt_FOUND)
        set(HAVE_LAYER_SHELL ON)
    endif()
endif()
message(STATUS "Layer shell support: ${HAVE_LAYER_SHELL}")

add_compile_options(-Wall -Wextra -Wpedantic)
add_compile_definitions(QT_NO_KEYWORDS QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII)

configure_file(src/config.h.in config.h @ONLY)

# Everything that does not need a window, so the tests can use it.
add_library(shuangpin-core STATIC
    src/composition.cpp
    src/composition.h
    src/key.h
    src/keyboardlayout.h
    src/pinyin.cpp
    src/pinyin.h
    src/practicestats.cpp
    src/practicestats.h
    src/scheme.cpp
    src/scheme.h
    src/schemelibrary.cpp
    src/schemelibrary.h
    src/typingtracker.cpp
    src/typingtracker.h
    src/visibilitypolicy.cpp
    src/visibilitypolicy.h)
target_include_directories(shuangpin-core PUBLIC src)
target_link_libraries(shuangpin-core PUBLIC Qt6::Core)

file(GLOB SCHEME_FILES RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" CONFIGURE_DEPENDS data/schemes/*.json)
qt_add_resources(shuangpin-core "schemes"
    PREFIX "/schemes"
    BASE data/schemes
    FILES ${SCHEME_FILES})

add_executable(shuangpin-overlay
    src/autostart.cpp
    src/autostart.h
    src/chartpainter.cpp
    src/chartpainter.h
    src/inputmethodwatcher.cpp
    src/inputmethodwatcher.h
    src/keyboardmonitor.cpp
    src/keyboardmonitor.h
    src/keyhighlights.cpp
    src/keyhighlights.h
    src/main.cpp
    src/overlaymenu.cpp
    src/overlaymenu.h
    src/overlaywindow.cpp
    src/overlaywindow.h
    src/settings.cpp
    src/settings.h
    src/windowplacement.cpp
    src/windowplacement.h)
target_include_directories(shuangpin-overlay PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
target_link_libraries(shuangpin-overlay PRIVATE shuangpin-core Qt6::Widgets Qt6::DBus)
if(HAVE_LAYER_SHELL)
    target_link_libraries(shuangpin-overlay PRIVATE LayerShellQt::Interface)
endif()

qt_add_translations(shuangpin-overlay
    TS_FILES translations/shuangpin-overlay_zh_CN.ts)

# The icon is also built in, for desktops that have not picked up the
# installed one yet.
qt_add_resources(shuangpin-overlay "icon"
    PREFIX "/icons"
    BASE data/icons
    FILES data/icons/app.svg)

install(TARGETS shuangpin-overlay RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
configure_file(data/app.desktop.in "${APP_ID}.desktop" @ONLY)
configure_file(data/app.metainfo.xml.in "${APP_ID}.metainfo.xml" @ONLY)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${APP_ID}.desktop"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${APP_ID}.metainfo.xml"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/metainfo")
install(FILES data/icons/app.svg
    DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps"
    RENAME "${APP_ID}.svg")

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
