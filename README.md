# LightMark — 跨平台 Markdown 编辑器

基于 **Qt6 + C++17** 的桌面 Markdown 编辑器，支持 Windows/Linux。

## 功能

- **Markdown 语法**：标题、粗体、斜体、行内代码、代码块、链接、图片、无序/有序列表、表格、引用
- **实时预览**：编辑区输入，预览区同步渲染，亮色/暗色主题切换
- **虚拟化编辑**：万行级大文档流畅编辑，只渲染可见区域
- **差分渲染**：编辑时保持预览区滚动位置不跳回顶部
- **查找替换**：支持大小写、全字匹配
- **编码检测**：自动识别 UTF-8/UTF-16/GBK 编码
- **导出**：HTML / PDF 导出
- **自动保存**：30 秒间隔 + 崩溃恢复

## 构建

### 依赖

- Qt 6.2+ (Core, Widgets, PrintSupport)
- CMake 3.19+
- C++17 编译器 (MSVC 2019+ / GCC 9+ / Clang 10+)

### 编译

```bash
git clone https://github.com/lzh1617055800/LightMark.git
cd LightMark
mkdir build && cd build
cmake .. -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/gcc_64
cmake --build .
```

Windows 下可直接用 Qt Creator 打开 `CMakeLists.txt`。

## 项目结构

```
├── main.cpp                    # 入口
├── mainwindow.h/cpp            # 主窗口，UI组装+业务逻辑 (Bug修复核心)
├── markdownparser.h/cpp        # Markdown→HTML 正则流水线解析引擎
├── editorhighlighter.h/cpp     # 编辑区语法高亮 (QSyntaxHighlighter)
├── virtualizededitor.h/cpp     # 虚拟化编辑区 (只渲染可见行)
├── diffrenderer.h/cpp          # 文档变化行号追踪
├── fileencodingdetector.h/cpp  # 文件编码检测 (UTF-8/16/GBK)
├── appsettings.h/cpp           # 配置持久化+崩溃恢复
└── CMakeLists.txt              # CMake 构建配置
```

## 架构

```
┌──────────┬──────────────────┬──────────────────┐
│ 行号区域  │  编辑区           │  预览区           │
│ QTextEdit │  VirtualizedEditor│  QTextEdit       │
│ (只读)    │  (可编辑)         │  (只读)           │
└──────────┴──────────────────┴──────────────────┘
```

- **编辑区**：VirtualizedEditor 继承 QTextEdit，内存中维护全文 (`m_fullContent`)，QTextDocument 只存可见区窗口。滚动时从全文切出新可见区。
- **预览区**：MarkdownParser 正则流水线转 HTML → 包裹主题 CSS → `setHtml()` 渲染。
- **数据流**：`用户编辑 → onTextChanged 合并回全文 → 200ms 防抖 → updatePreview 全量解析 → updateWordCount`

## Bug 修复记录

本项目经历了系统性的 Bug 分析修复，详细记录见 [面试准备.md](面试准备.md) 和 [BUG_ANALYSIS.md](BUG_ANALYSIS.md)。

## License

MIT
