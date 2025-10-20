@echo off
REM Windows构建脚本
REM 用于快速编译多媒体播放器项目

setlocal enabledelayedexpansion

REM 设置默认值
set "BUILD_TYPE=Release"
set "CLEAN=false"
set "INSTALL=false"
set "RUN=false"
set "VCPKG_TOOLCHAIN="
set "JOBS=4"

REM 解析命令行参数
:parse_args
if "%~1"=="" goto :check_requirements
if "%~1"=="-c" (
    set "CLEAN=true"
    shift
    goto :parse_args
)
if "%~1"=="--clean" (
    set "CLEAN=true"
    shift
    goto :parse_args
)
if "%~1"=="-d" (
    set "BUILD_TYPE=Debug"
    shift
    goto :parse_args
)
if "%~1"=="--debug" (
    set "BUILD_TYPE=Debug"
    shift
    goto :parse_args
)
if "%~1"=="-i" (
    set "INSTALL=true"
    shift
    goto :parse_args
)
if "%~1"=="--install" (
    set "INSTALL=true"
    shift
    goto :parse_args
)
if "%~1"=="-r" (
    set "RUN=true"
    shift
    goto :parse_args
)
if "%~1"=="--run" (
    set "RUN=true"
    shift
    goto :parse_args
)
if "%~1"=="-v" (
    set "VCPKG_TOOLCHAIN=%~2"
    shift
    shift
    goto :parse_args
)
if "%~1"=="--vcpkg" (
    set "VCPKG_TOOLCHAIN=%~2"
    shift
    shift
    goto :parse_args
)
if "%~1"=="-j" (
    set "JOBS=%~2"
    shift
    shift
    goto :parse_args
)
if "%~1"=="--jobs" (
    set "JOBS=%~2"
    shift
    shift
    goto :parse_args
)
if "%~1"=="-h" goto :show_help
if "%~1"=="--help" goto :show_help

echo 错误: 未知选项 %~1
goto :show_help

:show_help
echo 用法: %~nx0 [选项]
echo.
echo 选项:
echo   -c, --clean        清理构建目录
echo   -d, --debug        使用Debug模式编译
echo   -i, --install      编译后安装
echo   -r, --run          编译后运行
echo   -v, --vcpkg DIR    指定vcpkg工具链文件路径
echo   -j, --jobs N       使用N个并行任务编译
echo   -h, --help         显示此帮助信息
echo.
echo 环境变量:
echo   CMAKE_PREFIX_PATH  Qt安装路径
echo   VCPKG_ROOT        vcpkg根目录
echo.
echo 示例:
echo   %~nx0                              # 基本编译
echo   %~nx0 -c -d                        # 清理并Debug编译
echo   %~nx0 -v C:\vcpkg\scripts\buildsystems\vcpkg.cmake  # 使用vcpkg
echo   %~nx0 -i -r                        # 编译、安装并运行
goto :end

:print_info
echo [%TIME%] %~1
goto :eof

:print_error
echo [%TIME%] 错误: %~1
goto :eof

:print_success
echo [%TIME%] 成功: %~1
goto :eof

:check_requirements
call :print_info "开始构建多媒体播放器项目..."
call :print_info "构建类型: %BUILD_TYPE%"

REM 检查是否在项目根目录
if not exist "CMakeLists.txt" (
    call :print_error "请在项目根目录运行此脚本"
    goto :error
)

REM 检查cmake
where cmake >nul 2>&1
if %errorlevel% neq 0 (
    call :print_error "未找到cmake，请确保已安装CMake并添加到PATH"
    goto :error
)

REM 检查编译器
where cl >nul 2>&1
if %errorlevel% neq 0 (
    call :print_error "未找到MSVC编译器，请运行Visual Studio Developer Command Prompt"
    goto :error
)

call :print_success "基础工具检查通过"

:clean_build
if "%CLEAN%"=="true" (
    if exist "build" (
        call :print_info "清理构建目录..."
        rmdir /s /q build
        call :print_success "构建目录已清理"
    )
)

:configure_project
call :print_info "配置项目..."

if not exist "build" mkdir build
cd build

REM 构建CMake配置选项
set "CMAKE_OPTIONS=-DCMAKE_BUILD_TYPE=%BUILD_TYPE%"

REM 如果指定了vcpkg工具链
if not "%VCPKG_TOOLCHAIN%"=="" (
    set "CMAKE_OPTIONS=%CMAKE_OPTIONS% -DCMAKE_TOOLCHAIN_FILE=%VCPKG_TOOLCHAIN%"
) else if not "%VCPKG_ROOT%"=="" (
    set "CMAKE_OPTIONS=%CMAKE_OPTIONS% -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake"
)

REM 运行cmake配置
cmake %CMAKE_OPTIONS% ..
if %errorlevel% neq 0 (
    call :print_error "项目配置失败"
    goto :error
)

call :print_success "项目配置成功"
cd ..

:build_project
call :print_info "编译项目..."

cd build

REM 编译项目
cmake --build . --config %BUILD_TYPE% --parallel %JOBS%
if %errorlevel% neq 0 (
    call :print_error "项目编译失败"
    goto :error
)

call :print_success "项目编译成功"
cd ..

:install_project
if "%INSTALL%"=="true" (
    call :print_info "安装项目..."
    
    cd build
    cmake --install . --config %BUILD_TYPE%
    if %errorlevel% neq 0 (
        call :print_error "项目安装失败"
        goto :error
    )
    call :print_success "项目安装成功"
    cd ..
)

:run_project
if "%RUN%"=="true" (
    call :print_info "运行项目..."
    
    if exist "build\%BUILD_TYPE%\MultimediaPlayer.exe" (
        build\%BUILD_TYPE%\MultimediaPlayer.exe
    ) else if exist "build\MultimediaPlayer.exe" (
        build\MultimediaPlayer.exe
    ) else (
        call :print_error "可执行文件未找到"
        goto :error
    )
)

call :print_success "构建完成！"

if not "%RUN%"=="true" (
    if exist "build\%BUILD_TYPE%\MultimediaPlayer.exe" (
        call :print_info "运行程序: build\%BUILD_TYPE%\MultimediaPlayer.exe"
    ) else if exist "build\MultimediaPlayer.exe" (
        call :print_info "运行程序: build\MultimediaPlayer.exe"
    )
)

goto :end

:error
exit /b 1

:end
endlocal