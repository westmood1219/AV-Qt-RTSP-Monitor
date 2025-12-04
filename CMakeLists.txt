cmake_minimum_required(VERSION 3.16)

project(AV VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        VideoDemuxer.h
        VideoDemuxer.cpp
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(AV
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        DecodeThread.h
        DecodeThread.cpp
        Terminal查字典.md
        tempValue.txt
        rtsp_player.h rtsp_player.cpp
        videogridlayout.h videogridlayout.cpp
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET AV APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(AV SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(AV
            ${PROJECT_SOURCES}
        )
    endif()
endif()

set(FFMPEG_ROOT "D:/ffmpeg_env/ffmpeg-n8.0-latest-win64-gpl-shared-8.0")

# 调试：打印一下，确保路径是对的
message(STATUS "FFMPEG INCLUDE: ${FFMPEG_ROOT}/include")
message(STATUS "FFMPEG LIB: ${FFMPEG_ROOT}/lib")

# ========================================================
# 2. 告诉 CMake 库文件在哪里 (相当于 -L)
#    注意：必须在 add_executable 之后，target_link_libraries 之前
# ========================================================
target_link_directories(AV PRIVATE ${FFMPEG_ROOT}/lib)

# ========================================================
# 3. 告诉 CMake 头文件在哪里 (相当于 -I)
# ========================================================
target_include_directories(AV PRIVATE ${FFMPEG_ROOT}/include)

# ========================================================
# 4. 链接库 (相当于 -l)
#    MinGW 会自动优先去找 libavcodec.dll.a，这正是我们想要的
# ========================================================
target_link_libraries(AV PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    avcodec
    avformat
    avutil
    swscale
    swresample
)
target_link_libraries(AV PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.AV)
endif()
set_target_properties(AV PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS AV
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(AV)
endif()

message(STATUS "我的库安装路径是: ${CMAKE_INSTALL_LIBDIR}")
message(STATUS "当前构建类型是: ${CMAKE_BUILD_TYPE}")
message(STATUS "源代码目录在: ${CMAKE_CURRENT_SOURCE_DIR}")
