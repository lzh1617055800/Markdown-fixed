# LightMark — 跨平台 Markdown 编辑器（集成 AI 写作助手）

基于 **Qt6 + C++17** 的桌面 Markdown 编辑器，支持 Windows/Linux 跨平台。内置 DeepSeek AI 助手，可进行自然语言对话、文档润色、翻译等操作。

## 功能

### 编辑与预览
- **Markdown 语法**：标题、粗体/斜体、行内代码、代码块、链接、图片、列表、表格、引用
- **实时预览**：编辑区输入 → 预览区同步渲染 HTML，亮色/暗色主题切换
- **虚拟化编辑**：QTextDocument 仅承载可见行附近文本，内存维护全文模型，大文档流畅滚动
- **差分变更追踪**：记录文档变化行，配合防抖刷新保持预览滚动位置
- **语法高亮**：编辑区 QSyntaxHighlighter + 预览区 CSS 主题，支持明暗切换

### 文件与编码
- **多编码检测**：自动识别 UTF-8 / UTF-16（LE/BE）/ GBK，避免中文乱码
- **自动保存**：30 秒间隔 + 崩溃恢复检测
- **HTML / PDF 导出**

### AI 写作助手
- **自由对话**：底部面板一问一答，支持多轮对话
- **打字机效果**：SSE 流式响应逐字渲染
- **快捷操作**：润色、扩写、翻译英文、总结（点击填入提示词，可自由修改）
- **可替换后端**：基于 AiClient 抽象接口，当前接入 DeepSeek API（OpenAI 兼容格式），可替换为 GPT / 文心一言等

## 项目结构

```
├── include/                    # 头文件
│   ├── mainwindow.h            # 主窗口
│   ├── markdownparser.h        # Markdown → HTML 正则流水线解析器
│   ├── markdownhighlighter.h   # 预览区高亮（CSS主题）
│   ├── editorhighlighter.h     # 编辑区语法高亮（QSyntaxHighlighter）
│   ├── virtualizededitor.h     # 虚拟化编辑器（仅渲染可见行）
│   ├── diffrenderer.h          # 文档变更行追踪
│   ├── fileencodingdetector.h  # 文件编码检测（UTF-8/16/GBK）
│   ├── appsettings.h           # 配置持久化 + 自动保存恢复
│   ├── chatbubble.h            # 对话气泡 Widget
│   ├── ChatModel.h             # 对话历史数据模型
│   ├── aiclient.h              # AI 客户端抽象接口
│   ├── deepseekclient.h        # DeepSeek API 客户端（SSE 流）
│   ├── chatpanel.h             # AI 助手底部面板
│   └── contextbuilder.h        # AI 上下文构造器
│
├── src/                        # 源文件
│   ├── main.cpp                # 入口
│   ├── mainwindow.cpp          # 主窗口（UI 组装 + 业务逻辑）
│   ├── markdownparser.cpp      # Markdown 解析实现
│   ├── markdownhighlighter.cpp
│   ├── editorhighlighter.cpp
│   ├── virtualizededitor.cpp   # 虚拟化编辑实现
│   ├── diffrenderer.cpp
│   ├── fileencodingdetector.cpp
│   ├── appsettings.cpp
│   ├── chatbubble.cpp          # 气泡 UI 实现
│   ├── ChatModel.cpp           # 对话历史实现
│   ├── deepseekclient.cpp      # API 调用 + SSE 解析实现
│   ├── chatpanel.cpp           # AI 面板实现
│   ├── contextbuilder.cpp      # 上下文构造实现
│   └── mainwindow.ui           # Qt UI 布局文件
│
├── docs/                       # 开发文档
├── CMakeLists.txt              # CMake 构建配置
└── README.md                   # 本文件
```

## 架构

```
┌────────────────────────────────────────────────────────┐
│  MainWindow                                            │
│  ┌──────────────────────────────────────────────────┐ │
│  │  QSplitter (横向): 行号 | VirtualizedEditor | 预览 │ │
│  └──────────────────────────────────────────────────┘ │
│  ┌──────────────────────────────────────────────────┐ │
│  │  ChatPanel: [润色][扩写][翻译][总结]              │ │
│  │  气泡列表 (QScrollArea)                           │ │
│  │  输入框 + 发送按钮                                │ │
│  └──────────────────────────────────────────────────┘ │
└────────────────────────────────────────────────────────┘

数据流:
  用户输入 → ChatPanel → ContextBuilder（拼接上下文）
    → DeepSeekClient（HTTPS POST + SSE 解析）
    → streamChunk 信号 → ChatPanel 逐字渲染气泡
    → responseDone → 历史存入 ChatModel
```

## 构建

### 依赖

- **Qt 6.2+**（Core / Widgets / PrintSupport / Network）
- **CMake 3.19+**
- **C++17** 编译器（MSVC 2019+ / GCC 9+ / Clang 10+）

### 编译

```bash
git clone https://github.com/lzh1617055800/LightMark.git
cd LightMark
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/gcc_64
cmake --build .
```

Windows 下用 Qt Creator 打开 `CMakeLists.txt` 直接编译。

### 配置 AI 助手

1. 注册 [DeepSeek 开放平台](https://platform.deepseek.com)，获取 API Key
2. 在可执行文件同目录下创建 `deepseek_key.conf`，写入 API Key（一行纯文本）
3. 启动程序，菜单 View → AI 助手打开面板

## License

MIT
