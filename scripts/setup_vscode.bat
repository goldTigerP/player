@echo off
REM VS Code 自动配置脚本 (Windows)
REM 用于帮助 Windows 用户设置 VS Code 配置

setlocal enabledelayedexpansion

echo 🔧 正在为 VS Code 生成配置文件...

REM 检查是否在项目根目录
if not exist "CMakeLists.txt" (
    echo ❌ 错误: 请在项目根目录运行此脚本
    pause
    exit /b 1
)

REM 创建 .vscode 目录
if not exist ".vscode" mkdir .vscode

echo 📝 生成 VS Code 配置文件...

REM 生成 c_cpp_properties.json
echo { > .vscode\c_cpp_properties.json
echo     "configurations": [ >> .vscode\c_cpp_properties.json
echo         { >> .vscode\c_cpp_properties.json
echo             "name": "Windows", >> .vscode\c_cpp_properties.json
echo             "includePath": [ >> .vscode\c_cpp_properties.json
echo                 "${workspaceFolder}/include", >> .vscode\c_cpp_properties.json
echo                 "${workspaceFolder}/include/core", >> .vscode\c_cpp_properties.json
echo                 "${workspaceFolder}/include/ui",  >> .vscode\c_cpp_properties.json
echo                 "${workspaceFolder}/include/media", >> .vscode\c_cpp_properties.json
echo                 "请根据你的 Qt 安装路径更新以下路径：", >> .vscode\c_cpp_properties.json
echo                 "C:/Qt/6.*/msvc2019_64/include", >> .vscode\c_cpp_properties.json
echo                 "C:/Qt/6.*/msvc2019_64/include/QtCore", >> .vscode\c_cpp_properties.json
echo                 "C:/Qt/6.*/msvc2019_64/include/QtWidgets", >> .vscode\c_cpp_properties.json
echo                 "C:/Qt/6.*/msvc2019_64/include/QtGui", >> .vscode\c_cpp_properties.json
echo                 "C:/Qt/6.*/msvc2019_64/include/QtMultimedia", >> .vscode\c_cpp_properties.json
echo                 "请根据你的 OpenCV 安装路径更新：", >> .vscode\c_cpp_properties.json
echo                 "C:/opencv/build/include", >> .vscode\c_cpp_properties.json
echo                 "请根据你的 FFmpeg 安装路径更新：", >> .vscode\c_cpp_properties.json
echo                 "C:/ffmpeg/include" >> .vscode\c_cpp_properties.json
echo             ], >> .vscode\c_cpp_properties.json
echo             "defines": [ >> .vscode\c_cpp_properties.json
echo                 "_DEBUG", >> .vscode\c_cpp_properties.json
echo                 "UNICODE", >> .vscode\c_cpp_properties.json
echo                 "_UNICODE", >> .vscode\c_cpp_properties.json
echo                 "QT_CORE_LIB", >> .vscode\c_cpp_properties.json
echo                 "QT_WIDGETS_LIB",  >> .vscode\c_cpp_properties.json
echo                 "QT_GUI_LIB", >> .vscode\c_cpp_properties.json
echo                 "PLATFORM_WINDOWS" >> .vscode\c_cpp_properties.json
echo             ], >> .vscode\c_cpp_properties.json
echo             "windowsSdkVersion": "10.0.19041.0", >> .vscode\c_cpp_properties.json
echo             "compilerPath": "cl.exe", >> .vscode\c_cpp_properties.json
echo             "cStandard": "c17", >> .vscode\c_cpp_properties.json
echo             "cppStandard": "c++17", >> .vscode\c_cpp_properties.json
echo             "intelliSenseMode": "windows-msvc-x64" >> .vscode\c_cpp_properties.json
echo         } >> .vscode\c_cpp_properties.json
echo     ], >> .vscode\c_cpp_properties.json
echo     "version": 4 >> .vscode\c_cpp_properties.json
echo } >> .vscode\c_cpp_properties.json

REM 生成 settings.json
echo { > .vscode\settings.json
echo     "cmake.buildDirectory": "${workspaceFolder}/build", >> .vscode\settings.json
echo     "cmake.configureArgs": [ >> .vscode\settings.json
echo         "-DCMAKE_BUILD_TYPE=Debug", >> .vscode\settings.json
echo         "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON" >> .vscode\settings.json
echo     ], >> .vscode\settings.json
echo     "cmake.generator": "Visual Studio 16 2019", >> .vscode\settings.json
echo     "files.associations": { >> .vscode\settings.json
echo         "*.h": "cpp", >> .vscode\settings.json
echo         "*.cpp": "cpp", >> .vscode\settings.json
echo         "*.cmake": "cmake", >> .vscode\settings.json
echo         "CMakeLists.txt": "cmake" >> .vscode\settings.json
echo     }, >> .vscode\settings.json
echo     "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools", >> .vscode\settings.json
echo     "C_Cpp.default.intelliSenseMode": "windows-msvc-x64", >> .vscode\settings.json
echo     "C_Cpp.default.cppStandard": "c++17", >> .vscode\settings.json
echo     "C_Cpp.default.compilerPath": "cl.exe", >> .vscode\settings.json
echo     "C_Cpp.errorSquiggles": "enabled" >> .vscode\settings.json
echo } >> .vscode\settings.json

echo ✅ VS Code 配置文件生成完成！
echo.
echo 📋 下一步操作：
echo 1. 手动编辑 .vscode\c_cpp_properties.json 中的路径
echo 2. 重新加载 VS Code 窗口: Ctrl+Shift+P -^> 'Developer: Reload Window'
echo 3. 重置 C++ 智能感知: Ctrl+Shift+P -^> 'C/C++: Reset IntelliSense Database'
echo.
echo 🔧 重要提醒：
echo - 请根据你的实际安装路径修改配置文件中的路径
echo - 确保 Qt、OpenCV、FFmpeg 已正确安装
echo - 查看 .vscode\SETUP.md 获取详细说明
echo.

pause