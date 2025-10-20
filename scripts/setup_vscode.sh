#!/bin/bash

# VS Code 自动配置脚本
# 用于自动检测系统路径并生成 VS Code 配置文件

set -e

echo "🔧 正在为 VS Code 生成配置文件..."

# 检查是否在项目根目录
if [ ! -f "CMakeLists.txt" ]; then
    echo "❌ 错误: 请在项目根目录运行此脚本"
    exit 1
fi

# 创建 .vscode 目录
mkdir -p .vscode

# 检测 Qt5 路径
echo "🔍 检测 Qt5 路径..."
if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists Qt5Core; then
    QT5_CFLAGS=$(pkg-config --cflags Qt5Core Qt5Widgets Qt5Multimedia)
    QT5_INCLUDE_PATHS=$(echo "$QT5_CFLAGS" | grep -o '\-I[^ ]*' | sed 's/^-I//' | sort -u)
    echo "✅ 找到 Qt5 路径"
else
    echo "⚠️  警告: 未找到 Qt5，请手动配置路径"
    QT5_INCLUDE_PATHS=""
fi

# 检测 OpenCV 路径
echo "🔍 检测 OpenCV 路径..."
if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists opencv4; then
    OPENCV_CFLAGS=$(pkg-config --cflags opencv4)
    OPENCV_INCLUDE_PATHS=$(echo "$OPENCV_CFLAGS" | grep -o '\-I[^ ]*' | sed 's/^-I//' | sort -u)
    echo "✅ 找到 OpenCV 路径"
else
    echo "⚠️  警告: 未找到 OpenCV，请手动配置路径"
    OPENCV_INCLUDE_PATHS=""
fi

# 检测 FFmpeg 路径
echo "🔍 检测 FFmpeg 路径..."
if command -v pkg-config >/dev/null 2>&1 && pkg-config --exists libavcodec; then
    FFMPEG_CFLAGS=$(pkg-config --cflags libavcodec libavformat libavutil)
    FFMPEG_INCLUDE_PATHS=$(echo "$FFMPEG_CFLAGS" | grep -o '\-I[^ ]*' | sed 's/^-I//' | sort -u)
    echo "✅ 找到 FFmpeg 路径"
else
    echo "⚠️  警告: 未找到 FFmpeg，请手动配置路径"
    FFMPEG_INCLUDE_PATHS=""
fi

# 生成 c_cpp_properties.json
echo "📝 生成 c_cpp_properties.json..."

cat > .vscode/c_cpp_properties.json << EOF
{
    "configurations": [
        {
            "name": "Linux",
            "includePath": [
                "\${workspaceFolder}/include",
                "\${workspaceFolder}/include/core",
                "\${workspaceFolder}/include/ui", 
                "\${workspaceFolder}/include/media",
$(echo "$QT5_INCLUDE_PATHS" | sed 's/^/                "/' | sed 's/$/"/' | sed '$!s/$/,/')
$(echo "$OPENCV_INCLUDE_PATHS" | sed 's/^/                "/' | sed 's/$/"/' | sed '$!s/$/,/')
$(echo "$FFMPEG_INCLUDE_PATHS" | sed 's/^/                "/' | sed 's/$/"/' | sed '$!s/$/,/')
                "/usr/include",
                "/usr/local/include"
            ],
            "defines": [
                "QT_CORE_LIB",
                "QT_WIDGETS_LIB", 
                "QT_GUI_LIB",
                "QT_MULTIMEDIA_LIB",
                "QT_MULTIMEDIAWIDGETS_LIB",
                "QT_OPENGL_LIB",
                "PLATFORM_LINUX"
            ],
            "compilerPath": "/usr/bin/g++",
            "cStandard": "c17",
            "cppStandard": "c++17",
            "intelliSenseMode": "linux-gcc-x64",
            "compilerArgs": [
                "-std=c++17",
                "-Wall",
                "-Wextra"
            ],
            "configurationProvider": "ms-vscode.cmake-tools"
        }
    ],
    "version": 4
}
EOF

# 生成 settings.json
echo "📝 生成 settings.json..."

cat > .vscode/settings.json << EOF
{
    "cmake.buildDirectory": "\${workspaceFolder}/build",
    "cmake.configureArgs": [
        "-DCMAKE_BUILD_TYPE=Debug",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"
    ],
    "cmake.generator": "Unix Makefiles",
    "files.associations": {
        "*.h": "cpp",
        "*.cpp": "cpp",
        "*.cmake": "cmake",
        "CMakeLists.txt": "cmake"
    },
    "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
    "C_Cpp.default.intelliSenseMode": "linux-gcc-x64",
    "C_Cpp.default.cppStandard": "c++17",
    "C_Cpp.default.compilerPath": "/usr/bin/g++",
    "C_Cpp.errorSquiggles": "enabled",
    "files.exclude": {
        "build/": true,
        "**/*.o": true,
        "**/*.a": true,
        "**/*.so": true
    }
}
EOF

echo "✅ VS Code 配置文件生成完成！"
echo ""
echo "📋 下一步操作："
echo "1. 重新加载 VS Code 窗口: Ctrl+Shift+P -> 'Developer: Reload Window'"
echo "2. 重置 C++ 智能感知: Ctrl+Shift+P -> 'C/C++: Reset IntelliSense Database'"
echo "3. 选择配置: Ctrl+Shift+P -> 'C/C++: Select a Configuration' -> 'Linux'"
echo ""
echo "🔧 如果仍有问题，请检查："
echo "- 确保所有依赖库已正确安装"
echo "- 手动编辑 .vscode/c_cpp_properties.json 中的路径"
echo "- 查看 .vscode/SETUP.md 获取详细说明"