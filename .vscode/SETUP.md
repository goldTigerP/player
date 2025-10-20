# VS Code 配置说明

## 首次设置（重要）

本项目提供了跨平台的 VS Code 配置模板。由于不同系统和用户的库安装路径可能不同，需要手动创建本地配置文件。

## 自动配置

### Linux 用户
```bash
# 运行自动配置脚本
./scripts/setup_vscode.sh
```

### Windows 用户
```cmd
REM 运行自动配置脚本
scripts\setup_vscode.bat
```

注意：Windows 脚本会生成带有占位符的配置文件，你需要手动更新其中的路径。

## 手动配置

### 1. 创建本地配置文件

复制模板文件并根据你的系统进行配置：

```bash
# 复制配置模板
cp .vscode/c_cpp_properties.json.template .vscode/c_cpp_properties.json
cp .vscode/settings.json.template .vscode/settings.json
```

### 2. 获取系统路径信息

#### Linux/Ubuntu 系统：

```bash
# 查看 Qt5 头文件路径
pkg-config --cflags Qt5Core Qt5Widgets

# 查看 OpenCV 头文件路径  
pkg-config --cflags opencv4

# 查看 FFmpeg 头文件路径
pkg-config --cflags libavcodec libavformat
```

#### Windows 系统：

- Qt 路径通常在：`C:\Qt\6.x.x\msvc2019_64\include`
- OpenCV 路径通常在：`C:\opencv\build\include`  
- FFmpeg 路径根据安装方式而定

### 3. 更新配置文件

编辑 `.vscode/c_cpp_properties.json`，将模板中的路径替换为你系统的实际路径。

## 解决 Include 红色波浪线问题

我已经为这个项目配置了完整的VS Code开发环境。如果您仍然看到include的红色波浪线，请按以下步骤操作：

### 1. 重新加载VS Code窗口
- 按 `Ctrl+Shift+P` 打开命令面板
- 输入并选择 "Developer: Reload Window"

### 2. 重新配置C++智能感知
- 按 `Ctrl+Shift+P` 打开命令面板  
- 输入并选择 "C/C++: Reset IntelliSense Database"

### 3. 重新选择配置
- 按 `Ctrl+Shift+P` 打开命令面板
- 输入并选择 "C/C++: Select a Configuration"
- 选择 "Linux"

### 4. 确认CMake配置
- 按 `Ctrl+Shift+P` 打开命令面板
- 输入并选择 "CMake: Configure"
- 等待配置完成

## 文件说明

已创建的配置文件：

- `.vscode/c_cpp_properties.json` - C++智能感知配置
- `.vscode/settings.json` - VS Code项目设置
- `multimedia-player.code-workspace` - VS Code工作区文件
- `compile_commands.json` - 编译数据库（软链接到build目录）

## 头文件路径配置

包含的路径：
- 项目头文件：`include/`, `include/core/`, `include/ui/`, `include/media/`
- Qt5头文件：`/usr/include/x86_64-linux-gnu/qt5/`
- OpenCV头文件：`/usr/include/opencv4/`
- FFmpeg头文件：`/usr/include/x86_64-linux-gnu/`

## 如果问题仍然存在

1. 确保安装了VS Code C++扩展：
   ```bash
   code --install-extension ms-vscode.cpptools
   code --install-extension ms-vscode.cmake-tools
   ```

2. 检查是否所有依赖都已安装：
   ```bash
   pkg-config --cflags Qt5Core Qt5Widgets opencv4
   ```

3. 重新生成编译数据库：
   ```bash
   cd build
   cmake .. -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
   ```

4. 如果使用的是不同的Qt版本或安装路径，请修改 `.vscode/c_cpp_properties.json` 中的相应路径。

## 验证配置

打开任意头文件（如 `include/media/MediaPlayer.h`），检查：
- 语法高亮是否正常
- Include语句是否有红色波浪线
- 智能提示是否工作（按Ctrl+Space测试）

如果一切正常，您应该能看到正确的语法高亮和智能提示功能。