cmake_minimum_required(VERSION 3.16)
project(MultimediaPlayer VERSION 1.0.0 LANGUAGES CXX)

# C++ 标准设置
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 平台检测
if(WIN32)
    set(PLATFORM_NAME "Windows")
    add_compile_definitions(PLATFORM_WINDOWS)
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_NAME "Linux")
    add_compile_definitions(PLATFORM_LINUX)
elseif(APPLE)
    set(PLATFORM_NAME "macOS")
    add_compile_definitions(PLATFORM_MACOS)
endif()

message(STATUS "Configuring for platform: ${PLATFORM_NAME}")

# 设置构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 编译选项
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 包含自定义的CMake模块
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# 查找依赖包
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Multimedia MultimediaWidgets OpenGL)
find_package(PkgConfig REQUIRED)
find_package(OpenCV REQUIRED)

# 查找FFmpeg
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindFFmpeg.cmake)

# 设置Qt6自动处理
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# 包含目录
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include/core
    ${CMAKE_CURRENT_SOURCE_DIR}/include/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/include/media
    ${OpenCV_INCLUDE_DIRS}
    ${FFMPEG_INCLUDE_DIRS}
)

# 源文件收集
file(GLOB_RECURSE CORE_SOURCES "src/core/*.cpp")
file(GLOB_RECURSE UI_SOURCES "src/ui/*.cpp")
file(GLOB_RECURSE MEDIA_SOURCES "src/media/*.cpp")
file(GLOB_RECURSE HEADERS "include/**/*.h")

# 主程序源文件
set(MAIN_SOURCES
    src/main.cpp
    ${CORE_SOURCES}
    ${UI_SOURCES}
    ${MEDIA_SOURCES}
    ${HEADERS}
)

# 资源文件
qt6_add_resources(RESOURCES resources/resources.qrc)

# 创建可执行文件
qt6_add_executable(MultimediaPlayer ${MAIN_SOURCES} ${RESOURCES})

# 链接库
target_link_libraries(MultimediaPlayer
    Qt6::Core
    Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::OpenGL
    ${OpenCV_LIBS}
    ${FFMPEG_LIBRARIES}
)

# 平台特定的链接设置
if(WIN32)
    # Windows特定设置
    target_link_libraries(MultimediaPlayer ws2_32 secur32)
    set_target_properties(MultimediaPlayer PROPERTIES WIN32_EXECUTABLE TRUE)
elseif(UNIX AND NOT APPLE)
    # Linux特定设置
    target_link_libraries(MultimediaPlayer pthread dl)
endif()

# 安装规则
install(TARGETS MultimediaPlayer
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib/static
)

# 复制依赖的动态库（Windows）
if(WIN32)
    # 这里可以添加复制Qt和其他DLL的逻辑
endif()

# 打包设置
set(CPACK_PACKAGE_NAME "MultimediaPlayer")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Cross-platform multimedia player")
set(CPACK_PACKAGE_VENDOR "YourCompany")

if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
else()
    set(CPACK_GENERATOR "DEB;TGZ")
endif()

include(CPack)