#!/bin/bash

# Linux构建脚本
# 用于快速编译多媒体播放器项目

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印带颜色的消息
print_message() {
    echo -e "${2}[$(date '+%H:%M:%S')] $1${NC}"
}

print_error() {
    print_message "$1" "${RED}"
}

print_success() {
    print_message "$1" "${GREEN}"
}

print_info() {
    print_message "$1" "${BLUE}"
}

print_warning() {
    print_message "$1" "${YELLOW}"
}

# 检查命令是否存在
check_command() {
    if ! command -v "$1" &> /dev/null; then
        print_error "错误: 未找到命令 '$1'"
        return 1
    fi
    return 0
}

# 检查依赖
check_dependencies() {
    print_info "检查构建依赖..."
    
    local missing_deps=()
    
    if ! check_command "cmake"; then
        missing_deps+=("cmake")
    fi
    
    if ! check_command "make"; then
        missing_deps+=("build-essential")
    fi
    
    if ! check_command "pkg-config"; then
        missing_deps+=("pkg-config")
    fi
    
    # 检查Qt
    if ! pkg-config --exists Qt6Core; then
        missing_deps+=("qt6-base-dev qt6-multimedia-dev")
    fi
    
    # 检查FFmpeg
    if ! pkg-config --exists libavcodec; then
        missing_deps+=("libavcodec-dev libavformat-dev libavutil-dev")
    fi
    
    # 检查OpenCV
    if ! pkg-config --exists opencv4; then
        missing_deps+=("libopencv-dev")
    fi
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        print_error "缺少以下依赖:"
        for dep in "${missing_deps[@]}"; do
            echo "  - $dep"
        done
        echo
        print_info "请运行以下命令安装依赖:"
        echo "sudo apt update && sudo apt install ${missing_deps[*]}"
        return 1
    fi
    
    print_success "所有依赖已满足"
    return 0
}

# 清理构建目录
clean_build() {
    if [ -d "build" ]; then
        print_info "清理构建目录..."
        rm -rf build
        print_success "构建目录已清理"
    fi
}

# 配置项目
configure_project() {
    print_info "配置项目..."
    
    mkdir -p build
    cd build
    
    # CMAKE配置选项
    CMAKE_OPTIONS=(
        "-DCMAKE_BUILD_TYPE=${BUILD_TYPE:-Release}"
        "-DCMAKE_INSTALL_PREFIX=${INSTALL_PREFIX:-/usr/local}"
    )
    
    # 如果指定了Qt路径
    if [ -n "$QT_DIR" ]; then
        CMAKE_OPTIONS+=("-DCMAKE_PREFIX_PATH=$QT_DIR")
    fi
    
    # 运行cmake配置
    if cmake "${CMAKE_OPTIONS[@]}" ..; then
        print_success "项目配置成功"
    else
        print_error "项目配置失败"
        return 1
    fi
    
    cd ..
}

# 编译项目
build_project() {
    print_info "编译项目..."
    
    cd build
    
    # 获取CPU核心数用于并行编译
    CORES=$(nproc)
    
    if cmake --build . --parallel $CORES; then
        print_success "项目编译成功"
    else
        print_error "项目编译失败"
        return 1
    fi
    
    cd ..
}

# 安装项目
install_project() {
    if [ "$INSTALL" = "true" ]; then
        print_info "安装项目..."
        
        cd build
        
        if sudo cmake --install .; then
            print_success "项目安装成功"
        else
            print_error "项目安装失败"
            return 1
        fi
        
        cd ..
    fi
}

# 运行项目
run_project() {
    if [ "$RUN" = "true" ]; then
        print_info "运行项目..."
        
        if [ -f "build/MultimediaPlayer" ]; then
            ./build/MultimediaPlayer
        else
            print_error "可执行文件未找到"
            return 1
        fi
    fi
}

# 显示帮助信息
show_help() {
    echo "用法: $0 [选项]"
    echo
    echo "选项:"
    echo "  -c, --clean        清理构建目录"
    echo "  -d, --debug        使用Debug模式编译"
    echo "  -i, --install      编译后安装"
    echo "  -r, --run          编译后运行"
    echo "  -j, --jobs N       使用N个并行任务编译"
    echo "  -q, --qt-dir DIR   指定Qt安装目录"
    echo "  -h, --help         显示此帮助信息"
    echo
    echo "环境变量:"
    echo "  BUILD_TYPE         构建类型 (Release|Debug|RelWithDebInfo)"
    echo "  INSTALL_PREFIX     安装前缀 (默认: /usr/local)"
    echo "  QT_DIR            Qt安装目录"
    echo
    echo "示例:"
    echo "  $0                 # 基本编译"
    echo "  $0 -c -d           # 清理并Debug编译"
    echo "  $0 -i -r           # 编译、安装并运行"
}

# 主函数
main() {
    local CLEAN=false
    local INSTALL=false
    local RUN=false
    
    # 解析命令行参数
    while [[ $# -gt 0 ]]; do
        case $1 in
            -c|--clean)
                CLEAN=true
                shift
                ;;
            -d|--debug)
                BUILD_TYPE="Debug"
                shift
                ;;
            -i|--install)
                INSTALL=true
                shift
                ;;
            -r|--run)
                RUN=true
                shift
                ;;
            -j|--jobs)
                JOBS="$2"
                shift 2
                ;;
            -q|--qt-dir)
                QT_DIR="$2"
                shift 2
                ;;
            -h|--help)
                show_help
                exit 0
                ;;
            *)
                print_error "未知选项: $1"
                show_help
                exit 1
                ;;
        esac
    done
    
    print_info "开始构建多媒体播放器项目..."
    print_info "构建类型: ${BUILD_TYPE:-Release}"
    
    # 检查是否在项目根目录
    if [ ! -f "CMakeLists.txt" ]; then
        print_error "请在项目根目录运行此脚本"
        exit 1
    fi
    
    # 执行构建步骤
    if ! check_dependencies; then
        exit 1
    fi
    
    if [ "$CLEAN" = true ]; then
        clean_build
    fi
    
    if ! configure_project; then
        exit 1
    fi
    
    if ! build_project; then
        exit 1
    fi
    
    if ! install_project; then
        exit 1
    fi
    
    if ! run_project; then
        exit 1
    fi
    
    print_success "构建完成！"
    
    if [ "$RUN" != true ]; then
        print_info "运行程序: ./build/MultimediaPlayer"
    fi
}

# 运行主函数
main "$@"