cmake_minimum_required(VERSION 3.6)

project(TelegramDesktop)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/gyp ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Network)
get_target_property(QTCORE_INCLUDE_DIRS Qt5::Core INTERFACE_INCLUDE_DIRECTORIES)
list(GET QTCORE_INCLUDE_DIRS 0 QT_INCLUDE_DIR)

foreach(__qt_module IN ITEMS QtCore QtGui)
    list(APPEND QT_PRIVATE_INCLUDE_DIRS
        ${QT_INCLUDE_DIR}/${__qt_module}/${Qt5_VERSION})
    list(APPEND QT_PRIVATE_INCLUDE_DIRS
        ${QT_INCLUDE_DIR}/${__qt_module}/${Qt5_VERSION}/${__qt_module})
endforeach()
message(STATUS "Using Qt private include directories: ${QT_PRIVATE_INCLUDE_DIRS}")

find_package(PkgConfig REQUIRED)

pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libavutil libswresample libswscale)
pkg_check_modules(TGVOIP REQUIRED libtgvoip)
pkg_check_modules(LIBDRM REQUIRED libdrm)
pkg_check_modules(LIBVA REQUIRED libva-x11 libva-drm libva)
pkg_check_modules(MINIZIP REQUIRED minizip)

pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
pkg_check_modules(APPINDICATOR REQUIRED appindicator3-0.1)

find_package(ZLIB REQUIRED)

find_package(Threads REQUIRED)
find_package(OpenAL REQUIRED)
find_package(OpenSSL REQUIRED)
#find_package(LibLZMA REQUIRED)
find_package(X11 REQUIRED)

set(GSL_INCLUDE_DIR /usr/include/libmicrosoft-gsl)
set(VARIANT_INCLUDE_DIR /usr/include/libvariant)

set(TELEGRAM_SOURCES_DIR ${CMAKE_SOURCE_DIR}/SourceFiles)
set(TELEGRAM_RESOURCES_DIR ${CMAKE_SOURCE_DIR}/Resources)
include_directories(${TELEGRAM_SOURCES_DIR})

set(GENERATED_DIR ${CMAKE_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_DIR})

include(ExternalProject)

#find_package(Breakpad REQUIRED)

include(TelegramCodegenTools)
include(TelegramCodegen)

set(QRC_FILES
    Resources/qrc/telegram.qrc
    Resources/qrc/telegram_emoji.qrc
    Resources/qrc/telegram_emoji_large.qrc
    Resources/qrc/telegram_sounds.qrc
    # This only disables system plugin search path
    # We do not want this behavior for system build
    Resources/qrc/telegram_linux.qrc
)

file(GLOB FLAT_SOURCE_FILES SourceFiles/*.cpp SourceFiles/*.h)
# We do not want to include Qt plugins statically
list(REMOVE_ITEM FLAT_SOURCE_FILES ${CMAKE_SOURCE_DIR}/SourceFiles/qt_static_plugins.cpp)

file(GLOB_RECURSE SUBDIRS_SOURCE_FILES
    SourceFiles/qt_functions.cpp
    SourceFiles/base/*.cpp SourceFiles/base/*.h
    SourceFiles/boxes/*.cpp SourceFiles/boxes/*h
    SourceFiles/calls/*.cpp SourceFiles/calls/*.h
    SourceFiles/core/*.cpp SourceFiles/core/*.h
    SourceFiles/chat_helpers/*.cpp SourceFiles/chat_helpers/*.h
    SourceFiles/data/*.cpp SourceFiles/data/*.h
    SourceFiles/dialogs/*.cpp SourceFiles/dialogs/*.h
    SourceFiles/history/*.cpp SourceFiles/history/*.h
    SourceFiles/inline_bots/*.cpp SourceFiles/inline_bots/*.h
    SourceFiles/intro/*.cpp SourceFiles/intro/*.h
    SourceFiles/lang/*.cpp SourceFiles/lang/*.h
    SourceFiles/media/*.cpp SourceFiles/media/*.h
    SourceFiles/mtproto/*.cpp SourceFiles/mtproto/*.h
    SourceFiles/overview/*.cpp SourceFiles/overview/*.h
    SourceFiles/platform/linux/*.cpp SourceFiles/platform/linux/*.h
    SourceFiles/profile/*.cpp SourceFiles/profile/*.h
    SourceFiles/settings/*.cpp SourceFiles/settings/*.h
    SourceFiles/storage/*.cpp SourceFiles/storage/*.h
    SourceFiles/ui/*.cpp SourceFiles/ui/*.h
    SourceFiles/window/*.cpp SourceFiles/window/*.h)

add_executable(Telegram WIN32 ${QRC_FILES} ${FLAT_SOURCE_FILES} ${SUBDIRS_SOURCE_FILES})

target_link_libraries(Telegram
    Qt5::Widgets
    Qt5::Network
    Threads::Threads
    OpenSSL::SSL
    OpenSSL::Crypto
#    breakpad
    ${FFMPEG_LIBRARIES}
    ${LIBVA_LIBRARIES}
    ${LIBDRM_LIBRARIES}
    ${ZLIB_LIBRARY_RELEASE}
    ${GTK3_LIBRARIES}
    ${TGVOIP_LIBRARIES}
    ${MINIZIP_LIBRARIES}
    ${OPENAL_LIBRARY}
    ${APPINDICATOR_LIBRARIES}
#    ${LIBLZMA_LIBRARIES}
    ${X11_X11_LIB}
    )
target_include_directories(Telegram PUBLIC
    ${QT_PRIVATE_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_DIRS}
    ${LIBVA_INCLUDE_DIRS}
    ${LIBDRM_INCLUDE_DIRS}
    ${ZLIB_INCLUDE_DIR}
    ${GTK3_INCLUDE_DIRS}
    ${TGVOIP_INCLUDE_DIRS}
    ${GSL_INCLUDE_DIR}
    ${VARIANT_INCLUDE_DIR}
    ${MINIZIP_INCLUDE_DIRS}
    ${OPENAL_INCLUDE_DIR}
    ${APPINDICATOR_INCLUDE_DIRS}
#    ${LIBLZMA_INCLUDE_DIRS}
    ${GENERATED_DIR})

target_sources(Telegram PRIVATE ${TELEGRAM_GENERATED_SOURCES})
add_dependencies(Telegram telegram_codegen)

include(PrecompiledHeader)
add_precompiled_header(Telegram SourceFiles/stdafx.h)

target_compile_definitions(Telegram PUBLIC
    Q_OS_LINUX64
    CUSTOM_API_ID
    TDESKTOP_DISABLE_UNITY_INTEGRATION
    TDESKTOP_DISABLE_DESKTOP_FILE_GENERATION
    TDESKTOP_DISABLE_REGISTER_CUSTOM_SCHEME
    TDESKTOP_DISABLE_CRASH_REPORTS
    TDESKTOP_DISABLE_AUTOUPDATE
    __STDC_FORMAT_MACROS)

set_target_properties(Telegram PROPERTIES AUTOMOC_MOC_OPTIONS -bTelegram_pch/stdafx.h)

install(TARGETS Telegram RUNTIME DESTINATION bin)
install(PROGRAMS ${CMAKE_SOURCE_DIR}/../lib/xdg/telegram-desktop.desktop DESTINATION share/applications)
