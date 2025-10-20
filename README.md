# 多媒体播放器 (Multimedia Player)

一个基于 Qt、FFmpeg 和 OpenCV 的跨平台多媒体播放器，支持视频播放、音频播放和图片查看功能。

## 功能特性

### 🎥 视频播放
- 支持多种视频格式 (MP4, AVI, MKV, MOV, WMV 等)
- 播放控制 (播放/暂停/停止/前进/后退)
- 进度条拖拽定位
- 音量调节和静音
- 播放速度调节
- 全屏播放

### 🎵 音频播放
- 支持多种音频格式 (MP3, WAV, FLAC, AAC, OGG 等)
- 音频可视化显示
- 播放列表管理
- 随机播放和循环播放

### 🖼️ 图片查看
- 支持多种图片格式 (JPG, PNG, BMP, TIFF, GIF, WEBP 等)
- 图片缩放、旋转、翻转
- 图片亮度、对比度、饱和度调节
- 图片滤镜效果 (模糊、锐化、灰度等)

### 🎛️ 界面功能
- 现代化的用户界面
- 暗色和亮色主题切换
- 播放列表侧边栏
- 拖拽文件支持
- 快捷键支持
- 多语言界面

## 系统要求

### 支持的平台
- Ubuntu 20.04+ / Debian 11+
- Windows 10/11
- macOS 10.15+ (理论支持，未测试)

### 依赖库要求
- Qt 6.2+
- FFmpeg 4.4+
- OpenCV 4.5+
- CMake 3.16+
- C++17 编译器

## 开发环境配置

### VS Code 配置（推荐）

#### Linux 用户
```bash
# 自动配置 VS Code 开发环境
./scripts/setup_vscode.sh
```

#### Windows 用户
```cmd
REM 自动配置 VS Code 开发环境
scripts\setup_vscode.bat
```

注意：Windows 脚本会生成带有占位符的配置文件，你需要手动更新其中的路径。

详细配置说明请参考：[VS Code 配置指南](.vscode/SETUP.md)

#### 代码格式化
项目已配置自动代码格式化：
- **Tab 大小**: 4 个空格（自动转换）
- **保存时自动格式化**: 已启用
- **格式化工具**: clang-format (Google 风格)

详细说明请查看：[代码格式化配置](docs/CODE_FORMAT.md)

## 安装和编译

### Ubuntu/Debian 依赖安装

```bash
# 更新包管理器
sudo apt update

# 安装基础构建工具
sudo apt install build-essential cmake git

# 安装Qt6开发包
sudo apt install qt6-base-dev qt6-multimedia-dev qt6-tools-dev

# 安装FFmpeg开发包
sudo apt install libavcodec-dev libavformat-dev libavutil-dev libavfilter-dev libswscale-dev libswresample-dev

# 安装OpenCV开发包
sudo apt install libopencv-dev

# 安装其他依赖
sudo apt install pkg-config
```

### Windows 依赖安装

#### 方式1: 使用 vcpkg (推荐)
```powershell
# 安装vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
.\bootstrap-vcpkg.bat

# 安装依赖
.\vcpkg install qt6[core,widgets,multimedia] ffmpeg opencv --triplet x64-windows
```

#### 方式2: 手动安装
1. 从 [Qt官网](https://www.qt.io/download) 下载并安装 Qt 6.2+
2. 从 [FFmpeg官网](https://ffmpeg.org/download.html) 下载预编译的FFmpeg库
3. 从 [OpenCV官网](https://opencv.org/releases/) 下载预编译的OpenCV库

### 编译项目

```bash
# 克隆项目
git clone <repository_url>
cd player

# 创建构建目录
mkdir build && cd build

# 配置项目 (Linux/macOS)
cmake ..

# 配置项目 (Windows with vcpkg)
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg_root]/scripts/buildsystems/vcpkg.cmake

# 编译项目
cmake --build . --parallel

# 安装 (可选)
cmake --install . --prefix /usr/local
```

### 快速构建脚本

Linux/macOS:
```bash
./scripts/build_linux.sh
```

Windows:
```powershell
.\scripts\build_windows.bat
```

## 使用说明

### 基本操作

1. **打开媒体文件**
   - 菜单: 文件 → 打开文件 (Ctrl+O)
   - 拖拽文件到窗口
   
2. **播放控制**
   - 播放/暂停: 空格键 或 点击播放按钮
   - 停止: S键 或 点击停止按钮
   - 前一个/后一个: ←/→ 键 或 点击对应按钮
   
3. **音量控制**
   - 音量调节: 滚轮 或 拖拽音量条
   - 静音切换: M键 或 点击静音按钮

4. **视图控制**
   - 全屏切换: F11 或 双击播放区域
   - 缩放: Ctrl + 滚轮
   - 适合窗口: Ctrl+0

### 快捷键

| 功能 | 快捷键 |
|------|--------|
| 打开文件 | Ctrl+O |
| 播放/暂停 | 空格 |
| 停止 | S |
| 前一个 | ← |
| 后一个 | → |
| 静音切换 | M |
| 全屏切换 | F11 |
| 退出全屏 | Esc |
| 缩放放大 | Ctrl+= |
| 缩放缩小 | Ctrl+- |
| 适合窗口 | Ctrl+0 |
| 实际大小 | Ctrl+1 |

## 项目架构

```
player/
├── src/                    # 源代码
│   ├── core/              # 核心逻辑
│   ├── ui/                # 用户界面
│   ├── media/             # 媒体处理
│   └── main.cpp           # 程序入口
├── include/               # 头文件
│   ├── core/
│   ├── ui/
│   └── media/
├── resources/             # 资源文件
│   ├── icons/            # 图标
│   ├── stylesheets/      # 样式表
│   └── resources.qrc     # 资源清单
├── cmake/                 # CMake模块
├── docs/                  # 文档
├── build/                 # 构建目录
└── CMakeLists.txt        # 主CMake文件
```

## 开发信息

### 核心类设计

- **MediaPlayer**: FFmpeg视频/音频播放器
- **ImageViewer**: OpenCV图片查看器  
- **MediaManager**: 媒体文件管理器
- **MainWindow**: 主窗口界面
- **MediaDisplayWidget**: 媒体显示组件
- **PlaylistWidget**: 播放列表组件

### 技术栈

- **界面框架**: Qt 6 (Widgets)
- **视频处理**: FFmpeg
- **图片处理**: OpenCV
- **构建系统**: CMake
- **编程语言**: C++17

## 问题排除

### 常见问题

1. **编译错误: 找不到Qt库**
   ```bash
   # 设置Qt路径
   export CMAKE_PREFIX_PATH=/path/to/qt6
   ```

2. **编译错误: 找不到FFmpeg**
   ```bash
   # Ubuntu/Debian
   sudo apt install libavcodec-dev libavformat-dev libavutil-dev
   ```

3. **运行时错误: 缺少动态库**
   ```bash
   # Linux: 检查LD_LIBRARY_PATH
   export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
   
   # Windows: 确保DLL在PATH中或程序目录中
   ```

### 调试模式编译

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```

## 贡献指南

1. Fork 项目
2. 创建功能分支 (`git checkout -b feature/AmazingFeature`)
3. 提交更改 (`git commit -m 'Add some AmazingFeature'`)
4. 推送分支 (`git push origin feature/AmazingFeature`)
5. 创建 Pull Request

## 许可证

本项目采用 MIT 许可证。详见 [LICENSE](LICENSE) 文件。

## 致谢

- [Qt Project](https://www.qt.io/) - 跨平台应用框架
- [FFmpeg](https://ffmpeg.org/) - 多媒体处理库
- [OpenCV](https://opencv.org/) - 计算机视觉库

## 联系方式

- 项目主页: [GitHub Repository URL]
- 问题反馈: [GitHub Issues URL]
- 邮箱: your-email@example.com
