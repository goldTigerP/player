# 代码格式化配置说明

## 🎨 格式化配置概览

项目已配置完整的代码格式化设置，确保代码风格一致性。

## ⚙️ 配置详情

### Tab 设置
- **Tab 大小**: 4 个空格
- **插入空格**: 启用（不使用 Tab 字符）
- **自动检测缩进**: 禁用（强制使用项目设置）

### 自动格式化
- **保存时格式化**: ✅ 启用
- **粘贴时格式化**: ✅ 启用
- **输入时格式化**: ✅ 启用

### C++ 特定设置
- **格式化工具**: clang-format
- **基础风格**: Google 风格（自定义）
- **缩进宽度**: 4 个空格
- **行长度限制**: 100 字符

## 📁 配置文件位置

### VS Code 配置
- `.vscode/settings.json` - VS Code 编辑器设置
- `.vscode/c_cpp_properties.json` - C++ 智能感知配置

### clang-format 配置
- `.clang-format` - 项目根目录的格式化规则文件

## 🔧 格式化规则详情

### 缩进和空格
```cpp
// ✅ 正确：4 个空格缩进
class MediaPlayer {
    void play() {
        if (isReady) {
            startPlayback();
        }
    }
};

// ❌ 错误：Tab 字符或其他缩进
class MediaPlayer {
	void play() {  // Tab 字符
	  if (isReady) {  // 2 个空格
	        startPlayback();  // 混合缩进
	  }
	}
};
```

### 函数和控制结构
```cpp
// ✅ 正确：每个函数单独一行
void MediaPlayer::play() {
    // 实现代码
}

void MediaPlayer::pause() {
    // 实现代码
}

// ❌ 错误：单行函数（除非非常简单）
void play() { startPlayback(); }  // 不推荐
```

### 指针和引用对齐
```cpp
// ✅ 正确：指针和引用左对齐
int* ptr = nullptr;
const std::string& name = getName();

// ❌ 错误：右对齐
int *ptr = nullptr;
const std::string &name = getName();
```

### 大括号风格
```cpp
// ✅ 正确：Attach 风格
class MediaPlayer {
public:
    void play() {
        if (condition) {
            doSomething();
        }
    }
};

// ❌ 错误：Allman 风格
class MediaPlayer 
{
public:
    void play() 
    {
        if (condition) 
        {
            doSomething();
        }
    }
};
```

## 🚀 使用方法

### 在 VS Code 中格式化代码

1. **格式化当前文件**
   - 快捷键: `Shift + Alt + F`
   - 或命令面板: `Format Document`

2. **格式化选中代码**
   - 快捷键: `Ctrl + K, Ctrl + F`
   - 或命令面板: `Format Selection`

3. **自动格式化**
   - 保存文件时自动格式化
   - 粘贴代码时自动格式化
   - 输入时实时格式化

### 命令行格式化

```bash
# 格式化单个文件
clang-format -i src/main.cpp

# 格式化整个目录
find src include -name "*.cpp" -o -name "*.h" | xargs clang-format -i

# 检查格式化（不修改文件）
clang-format --dry-run --Werror src/main.cpp
```

## 🔍 验证格式化配置

### 1. 检查 VS Code 设置
- 打开 VS Code
- 按 `Ctrl + ,` 打开设置
- 搜索 "tab size" 确认为 4
- 搜索 "insert spaces" 确认已启用

### 2. 测试格式化
创建一个测试文件：
```cpp
// test_format.cpp
#include<iostream>
class Test{
public:
void   func(  ){
if(true)
{
std::cout<<"test"<<std::endl;
}
}
};
```

使用 `Shift + Alt + F` 格式化后应该变成：
```cpp
// test_format.cpp
#include <iostream>
class Test {
public:
    void func() {
        if (true) {
            std::cout << "test" << std::endl;
        }
    }
};
```

## ⚡ 提示和技巧

### 1. 格式化快捷键
- `Shift + Alt + F`: 格式化整个文档
- `Ctrl + K, Ctrl + F`: 格式化选中内容
- `Ctrl + Shift + I`: 自动缩进选中行

### 2. 禁用特定区域的格式化
```cpp
// clang-format off
int matrix[3][3] = {
    {1, 2, 3},
    {4, 5, 6},
    {7, 8, 9}
};
// clang-format on
```

### 3. 配置文件优先级
1. `.clang-format` (项目根目录)
2. VS Code settings.json 中的 `C_Cpp.clang_format_style`
3. 系统默认配置

## 🛠️ 故障排除

### 问题：格式化不工作
1. 确保安装了 clang-format:
   ```bash
   sudo apt install clang-format
   ```

2. 重新加载 VS Code 窗口:
   - `Ctrl + Shift + P` -> "Developer: Reload Window"

3. 检查 C++ 扩展是否正常工作

### 问题：格式化结果不符合预期
1. 检查 `.clang-format` 文件是否存在
2. 验证 VS Code 设置中的格式化配置
3. 尝试手动运行 clang-format 命令

---

🎉 **现在你的项目已经配置了完整的代码格式化支持！Tab 将被自动转换为 4 个空格，保存时会自动格式化代码。**