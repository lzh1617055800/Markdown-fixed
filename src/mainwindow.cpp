#include "MainWindow.h"
#include "MarkdownParser.h"
#include "editorhighlighter.h"
#include "appsettings.h"
#include "diffrenderer.h"
#include "virtualizededitor.h"
#include "fileencodingdetector.h"
//AI
#include "chatpanel.h"
#include "deepseekclient.h"

#include <QSplitter>
#include <QTextEdit>
#include <QMenuBar>
#include <QStatusBar>
#include <QToolBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QAction>
#include <QLabel>
#include <QTimer>
#include <QFont>
#include <QFontDatabase>
#include <QTextCursor>
#include <QTextDocument>
#include <QApplication>
#include <QClipboard>
#include <QPrinter>
#include <QLineEdit>        // 新增：搜索框
#include <QCheckBox>        // 新增：选项框
#include <QDialog>          // 新增：对话框
#include <QPushButton>      // 新增：按钮
#include <QVBoxLayout>      // 新增：布局
#include <QHBoxLayout>
#include <QGroupBox>
#include <QTextBlock>
#include <QScrollBar>
#include <QTextStream>
#include <QAbstractTextDocumentLayout>
#include <QDir>
#include <QDateTime>
#include <algorithm>

// 常量定义
constexpr int WINDOW_WIDTH = 1400;
constexpr int WINDOW_HEIGHT = 900;
constexpr int STATUS_BAR_TIMEOUT = 3000;
constexpr double ZOOM_STEP = 1.0;
constexpr double ZOOM_DEFAULT = 1.0;
constexpr int AUTO_SAVE_INTERVAL = 30000;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_splitter(nullptr)
    , m_editor(nullptr)
    , m_preview(nullptr)
    , m_lineNumberArea(nullptr)    // 新增初始化
    , m_findEdit(nullptr)
    , m_replaceEdit(nullptr)
    , m_statusLabel(nullptr)
    , m_cursorPositionLabel(nullptr)
    , m_fileInfoLabel(nullptr)
    , m_wordCountLabel(nullptr)    // 新增初始化
    , m_fileMenu(nullptr)
    , m_editMenu(nullptr)
    , m_viewMenu(nullptr)
    , m_helpMenu(nullptr)
    , m_toolBar(nullptr)
    , m_newAction(nullptr)
    , m_openAction(nullptr)
    , m_saveAction(nullptr)
    , m_saveAsAction(nullptr)
    , m_exportHtmlAction(nullptr)
    , m_exportPdfAction(nullptr)
    , m_exitAction(nullptr)
    , m_undoAction(nullptr)
    , m_redoAction(nullptr)
    , m_cutAction(nullptr)
    , m_copyAction(nullptr)
    , m_pasteAction(nullptr)
    , m_findAction(nullptr)
    , m_replaceAction(nullptr)
    , m_selectAllAction(nullptr)
    , m_togglePreviewAction(nullptr)
    , m_zoomInAction(nullptr)
    , m_zoomOutAction(nullptr)
    , m_resetZoomAction(nullptr)
    , m_toggleLineNumbersAction(nullptr)
    , m_toggleWordWrapAction(nullptr)
    , m_toggleSyntaxHighlightAction(nullptr)
    , m_toggleDarkThemeAction(nullptr)
    , m_aboutAction(nullptr)
    , m_editorHighlighter(nullptr)
    , m_autoSaveTimer(nullptr)
    , m_autoSaveDebounce(nullptr)
    , m_currentFile("")
    , m_autoSaveEnabled(true)
    , m_appSettings(new AppSettings(this))
    , m_isModified(false)
    , m_zoomFactor(ZOOM_DEFAULT)
    , m_previewVisible(true)
    , m_lineNumbersVisible(true)   // 默认显示行号
    , m_wordWrapEnabled(false)     // 默认不自动换行
    , m_syntaxHighlightEnabled(true) // 默认启用语法高亮
    , m_darkThemeEnabled(false)    // 默认亮色主题
    , m_wordCount(0)
    , m_charCount(0)
    , m_diffRenderer(new DiffRenderer(this))
    , m_updateTimer(new QTimer(this))
{
    setupUI();
    setupMenu();
    setupToolBar();
    setupStatusBar();
    setupSyntaxHighlighter();  // 新增：设置语法高亮
    setupLineNumbers();        // 新增：设置行号显示
    setupConnections();

    setWindowTitle("Markdown编辑器 - 新文件");
    resize(WINDOW_WIDTH, WINDOW_HEIGHT);

    // 初始状态更新
    updateStatusBar();
    updateWindowTitle();
    updateWordCount();    // 新增：更新字数统计
    checkAndPromptAutoSaveRecovery();

    m_autoSaveTempPath = QDir::tempPath() + QDir::separator() + "LightMark_AutoSave";
    QDir tempDir(m_autoSaveTempPath);
    if (!tempDir.exists()) {
        tempDir.mkpath(m_autoSaveTempPath); // 创建临时目录（若不存在）
        qDebug() << "自动保存临时目录已创建：" << m_autoSaveTempPath;
    }

    // 设置自动保存定时器
    m_autoSaveTimer = new QTimer(this);
    connect(m_autoSaveTimer, &QTimer::timeout, this, &MainWindow::autoSave);
    m_autoSaveTimer->start(AUTO_SAVE_INTERVAL);

    // Bug 18修复：用成员变量替代static QTimer
    m_autoSaveDebounce = new QTimer(this);
    m_autoSaveDebounce->setSingleShot(true);
    m_autoSaveDebounce->setInterval(1000);
    connect(m_autoSaveDebounce, &QTimer::timeout, this, &MainWindow::autoSave);

    m_diffRenderer->setDocument(m_editor->document());

    m_updateTimer->setSingleShot(true); // 仅触发一次
    m_updateTimer->setInterval(200);    // 延迟200ms（可调整）

    // Bug 12修复：去除重复调用
    connect(m_updateTimer, &QTimer::timeout, this, [this]() {
        updatePreview();
        updateWordCount();
        updateStatusBar();
    });
}
MainWindow::~MainWindow(){}

void MainWindow::setupUI()
{
    // 创建水平分割器
    m_splitter = new QSplitter(Qt::Horizontal, this);

    // 创建行号区域（放在最左侧）
    m_lineNumberArea = new QTextEdit();
    m_lineNumberArea->setReadOnly(true);
    m_lineNumberArea->setLineWrapMode(QTextEdit::NoWrap); // Bug 24修复：禁止行号换行
    m_lineNumberArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_lineNumberArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_lineNumberArea->setFrameShape(QFrame::NoFrame);
    m_lineNumberArea->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    m_lineNumberArea->setFixedWidth(35);
    m_splitter->addWidget(m_lineNumberArea);

    // 创建编辑器（中间）
    m_editor = new VirtualizedEditor();
    QFont editorFont("Consolas", 11);
    if (!QFontDatabase::hasFamily("Consolas")) {
        editorFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    }
    m_editor->setFont(editorFont);
    m_lineNumberArea->setFont(editorFont); // 确保行号区域使用相同字体
    m_editor->setPlaceholderText(
        "Markdown编辑器\n\n"
        "支持以下Markdown语法：\n"
        "# 一级标题\n"
        "## 二级标题\n"
        "### 三级标题\n\n"
        "**粗体文字** 和 *斜体文字*\n\n"
        "`行内代码` 和 代码块：\n"
        "```\n代码块内容\n```\n\n"
        "[链接文字](http://example.com)\n\n"
        "![图片描述](图片URL)\n\n"
        "- 无序列表项\n"
        "1. 有序列表项\n\n"
        "💡 开始写作吧..."
        );
    m_splitter->addWidget(m_editor); // 关键：添加到分割器

    // 创建预览区域（右侧）
    m_preview = new QTextEdit();
    m_preview->setReadOnly(true);
    m_preview->setPlaceholderText("HTML预览区域右侧将实时显示Markdown渲染结果");
    m_preview->setAcceptRichText(true);
    m_preview->setFont(editorFont);
    QString editorStyle = R"(
        QTextEdit {
            background-color: #ffffff;
            color:#000000;
            border: 1px solid #e9ecef;
            padding: 10px;
            line-height: 1.6;
        }
    )";
    m_preview->setStyleSheet(editorStyle);
    m_editor->setStyleSheet(editorStyle);
    m_splitter->addWidget(m_preview); // 关键：添加到分割器
    m_lineNumberArea->setStyleSheet(editorStyle);

    // 设置分割器比例
    m_splitter->setSizes({100, 500, 500});
    m_splitter->setHandleWidth(2);

    setupAIPanel();
    m_verticalSplitter = new QSplitter(Qt::Vertical,this);
    m_verticalSplitter->addWidget(m_splitter);
    m_verticalSplitter->addWidget(m_chatPanel);
    m_verticalSplitter->setStretchFactor(0,3);
    m_verticalSplitter->setStretchFactor(1,1);
    m_verticalSplitter->setHandleWidth(2);

    // 设置中心部件
    setCentralWidget(m_verticalSplitter);

    // 初始化行号显示
    updateLineNumbers();
}

void MainWindow::setupAIPanel()
{
    m_aiClient = new DeepSeekClient(this);
    m_chatPanel = new ChatPanel(m_aiClient, this);
}

void MainWindow::toggleAIPanel()
{
    m_aiPanelVisible = !m_aiPanelVisible;
    m_chatPanel->setVisible(m_aiPanelVisible);
    m_toggleAIAction->setChecked(m_aiPanelVisible);
}

void MainWindow::setupMenu()
{
    m_fileMenu = menuBar()->addMenu("文件(&F)");

    m_newAction = new QAction("新建(&N)", this);
    m_newAction->setShortcut(QKeySequence::New);
    m_fileMenu->addAction(m_newAction);

    m_openAction = new QAction("打开(&O)", this);
    m_openAction->setShortcut(QKeySequence::Open);
    m_fileMenu->addAction(m_openAction);

    m_saveAction = new QAction("保存(&S)", this);
    m_saveAction->setShortcut(QKeySequence::Save);
    m_fileMenu->addAction(m_saveAction);

    m_saveAsAction = new QAction("另存为(&A)", this);
    m_saveAsAction->setShortcut(QKeySequence::SaveAs);
    m_fileMenu->addAction(m_saveAsAction);

    m_exportHtmlAction = new QAction("导出为HTML(&H)", this);
    m_exportHtmlAction->setShortcut(QKeySequence("Ctrl+Shift+H"));
    m_fileMenu->addAction(m_exportHtmlAction);

    m_exportPdfAction = new QAction("导出为PDF(&P)", this);
    m_exportPdfAction->setShortcut(QKeySequence("Ctrl+Shift+P"));
    m_fileMenu->addAction(m_exportPdfAction);

    m_fileMenu->addSeparator();

    m_exitAction = new QAction("退出(&X)", this);
    m_exitAction->setShortcut(QKeySequence::Quit);
    m_fileMenu->addAction(m_exitAction);

    // === 编辑菜单 ===
    m_editMenu = menuBar()->addMenu("编辑(&E)");

    m_undoAction = new QAction("撤销(&U)", this);
    m_undoAction->setShortcut(QKeySequence::Undo);
    m_undoAction->setStatusTip("撤销上次操作");
    m_editMenu->addAction(m_undoAction);

    m_redoAction = new QAction("重做(&R)", this);
    m_redoAction->setShortcut(QKeySequence::Redo);
    m_redoAction->setStatusTip("重做上次撤销的操作");
    m_editMenu->addAction(m_redoAction);

    m_editMenu->addSeparator();

    m_findAction = new QAction("查找(&F)", this);
    m_findAction->setShortcut(QKeySequence::Find);
    m_findAction->setStatusTip("查找文本");
    m_editMenu->addAction(m_findAction);

    m_replaceAction = new QAction("替换(&R)", this);
    m_replaceAction->setShortcut(QKeySequence::Replace);
    m_replaceAction->setStatusTip("查找并替换文本");
    m_editMenu->addAction(m_replaceAction);


    m_editMenu->addSeparator();

    m_selectAllAction = new QAction("全选(&A)", this);
    m_selectAllAction->setShortcut(QKeySequence::SelectAll);
    m_editMenu->addAction(m_selectAllAction);

    m_editMenu->addSeparator();

    m_cutAction = new QAction("剪切(&T)", this);
    m_cutAction->setShortcut(QKeySequence::Cut);
    m_cutAction->setStatusTip("剪切选中文本");
    m_editMenu->addAction(m_cutAction);

    m_copyAction = new QAction("复制(&C)", this);
    m_copyAction->setShortcut(QKeySequence::Copy);
    m_copyAction->setStatusTip("复制选中文本");
    m_editMenu->addAction(m_copyAction);

    m_pasteAction = new QAction("粘贴(&P)", this);
    m_pasteAction->setShortcut(QKeySequence::Paste);
    m_pasteAction->setStatusTip("粘贴剪贴板内容");
    m_editMenu->addAction(m_pasteAction);

    // === 视图菜单 ===
    m_viewMenu = menuBar()->addMenu("视图(&V)");

    m_togglePreviewAction = new QAction("切换预览(&P)", this);
    m_togglePreviewAction->setShortcut(QKeySequence("F9"));
    m_togglePreviewAction->setStatusTip("显示/隐藏预览面板");
    m_togglePreviewAction->setCheckable(true);
    m_togglePreviewAction->setChecked(true);
    m_viewMenu->addAction(m_togglePreviewAction);

    m_zoomInAction = new QAction("放大(&I)", this);
    m_zoomInAction->setShortcut(QKeySequence::ZoomIn);
    m_viewMenu->addAction(m_zoomInAction);

    m_zoomOutAction = new QAction("缩小(&O)", this);
    m_zoomOutAction->setShortcut(QKeySequence::ZoomOut);
    m_viewMenu->addAction(m_zoomOutAction);

    m_resetZoomAction = new QAction("重置缩放(&R)", this);
    m_resetZoomAction->setShortcut(QKeySequence("Ctrl+0"));
    m_viewMenu->addAction(m_resetZoomAction);

    m_viewMenu->addSeparator();

    m_toggleLineNumbersAction = new QAction("显示行号(&L)", this);
    m_toggleLineNumbersAction->setShortcut(QKeySequence("F11"));
    m_toggleLineNumbersAction->setStatusTip("显示/隐藏行号");
    m_toggleLineNumbersAction->setCheckable(true);
    m_toggleLineNumbersAction->setChecked(true);
    m_viewMenu->addAction(m_toggleLineNumbersAction);

    m_toggleWordWrapAction = new QAction("自动换行(&W)", this);
    m_toggleWordWrapAction->setShortcut(QKeySequence("F12"));
    m_toggleWordWrapAction->setStatusTip("启用/禁用自动换行");
    m_toggleWordWrapAction->setCheckable(true);
    m_toggleWordWrapAction->setChecked(false);
    m_viewMenu->addAction(m_toggleWordWrapAction);

    m_toggleSyntaxHighlightAction = new QAction("语法高亮(&H)", this);
    m_toggleSyntaxHighlightAction->setStatusTip("启用/禁用语法高亮");
    m_toggleSyntaxHighlightAction->setCheckable(true);
    m_toggleSyntaxHighlightAction->setChecked(true);
    m_viewMenu->addAction(m_toggleSyntaxHighlightAction);

    m_toggleDarkThemeAction = new QAction("暗色主题(&D)", this);
    m_toggleDarkThemeAction->setStatusTip("切换亮色/暗色主题");
    m_toggleDarkThemeAction->setCheckable(true);
    m_toggleDarkThemeAction->setChecked(false);
    m_viewMenu->addAction(m_toggleDarkThemeAction);

    m_toggleAIAction = new QAction("AI 助手(&I)", this);
    m_toggleAIAction->setStatusTip("显示/隐藏 AI 写作助手");
    m_toggleAIAction->setCheckable(true);
    m_toggleAIAction->setChecked(true);
    m_viewMenu->addAction(m_toggleAIAction);

    QAction *toggleDiffAction = new QAction("启用差分渲染(&D)", this);
    toggleDiffAction->setCheckable(true);
    toggleDiffAction->setChecked(m_diffRenderEnabled); // 默认启用
    connect(toggleDiffAction, &QAction::toggled, this, [this](bool checked) {
        m_diffRenderEnabled = checked;
        m_statusLabel->setText(checked ? "已启用差分渲染" : "已禁用差分渲染");
    });
    m_viewMenu->addAction(toggleDiffAction); // 假设已有m_viewMenu（视图菜单）


    // === 帮助菜单 ===
    m_helpMenu = menuBar()->addMenu("帮助(&H)");

    m_aboutAction = new QAction("关于(&A)", this);
    m_aboutAction->setStatusTip("显示关于对话框");
    m_helpMenu->addAction(m_aboutAction);
}

void MainWindow::setupToolBar()
{
    m_toolBar = addToolBar("主工具栏");
    m_toolBar->setMovable(false);

    // Bug 11修复：使用成员变量而非菜单索引
    m_toolBar->addAction(m_newAction);
    m_toolBar->addAction(m_openAction);
    m_toolBar->addAction(m_saveAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_undoAction);
    m_toolBar->addAction(m_redoAction);
    m_toolBar->addSeparator();
    m_toolBar->addAction(m_cutAction);
    m_toolBar->addAction(m_copyAction);
    m_toolBar->addAction(m_pasteAction);
}

void MainWindow::setupStatusBar()
{
    // 创建状态栏标签
    m_statusLabel = new QLabel("就绪");
    m_cursorPositionLabel = new QLabel("行: 1, 列: 1");
    m_fileInfoLabel = new QLabel("新文件");

    // 添加到状态栏
    statusBar()->addWidget(m_statusLabel, 1);  // 拉伸因子为1
    statusBar()->addPermanentWidget(m_cursorPositionLabel);
    statusBar()->addPermanentWidget(m_fileInfoLabel);

    m_wordCountLabel = new QLabel("字数: 0 | 字符: 0");
    statusBar()->addPermanentWidget(m_wordCountLabel);
}

void MainWindow::setupSyntaxHighlighter()
{
    // Bug 3修复：只给编辑区设置高亮器（编辑区含Markdown语法，预览区是HTML用CSS）
    if (m_editor && m_editor->document()) {
        m_editorHighlighter = new EditorHighlighter(m_editor->document());
        if (m_darkThemeEnabled) {
            m_editorHighlighter->setTheme("dark");
        } else {
            m_editorHighlighter->setTheme("light");
        }
        qDebug() << "编辑区高亮器初始化完成";
    } else {
        qDebug() << "无法初始化编辑区高亮器：编辑器或文档为空";
    }
    // 预览区样式完全由updatePreview()中的CSS控制，不需要QSyntaxHighlighter
}

void MainWindow::setupLineNumbers()
{
    // 确保使用与编辑器相同的字体
    QFont font = m_editor->font();
    m_lineNumberArea->setFont(font);

    // 行号区域样式
    QString lineNumberStyle = R"(
        QTextEdit {
            background-color: #ffffff;
            color:#000000;
            border: 1px solid #e9ecef;
            padding: 10px;
            line-height: 1.6;
        }
    )";
    m_lineNumberArea->setStyleSheet(lineNumberStyle);

    // 设置行高（关键修改：使用默认文本块格式，确保所有新旧文本块生效）
    QString editorStyle = R"(
        QTextEdit {
            line-height: 1.5;
        }
    )";
    m_editor->document()->setDefaultStyleSheet(editorStyle);

    // 行号区域：通过样式表设置行高（与编辑器一致）
    QString lineNumStyle = R"(
        QTextEdit {
            line-height: 1.5;
        }
    )";
    m_lineNumberArea->document()->setDefaultStyleSheet(lineNumStyle);

    updateLineNumbers();
    // 连接信号
    connect(m_editor->verticalScrollBar(), &QScrollBar::valueChanged,
            this, &MainWindow::updateLineNumbers);
    connect(m_editor, &QTextEdit::textChanged,
            this, &MainWindow::updateLineNumbers);
    connect(m_editor->document()->documentLayout(), &QAbstractTextDocumentLayout::update,
            this, &MainWindow::updateLineNumbers);
    connect(m_editor->verticalScrollBar(), &QScrollBar::rangeChanged,
            this, &MainWindow::updateLineNumbers);
}

void MainWindow::setupConnections()
{
    // Bug 11修复：全部通过成员变量连接，不依赖菜单索引

    // 文件菜单
    connect(m_newAction, &QAction::triggered, this, &MainWindow::newFile);
    connect(m_openAction, &QAction::triggered, this, &MainWindow::openFile);
    connect(m_saveAction, &QAction::triggered, this, &MainWindow::saveFile);
    connect(m_saveAsAction, &QAction::triggered, this, &MainWindow::saveAs);
    connect(m_exportHtmlAction, &QAction::triggered, this, &MainWindow::exportToHtml);
    connect(m_exportPdfAction, &QAction::triggered, this, &MainWindow::exportToPdf);
    connect(m_exitAction, &QAction::triggered, this, &QWidget::close);

    // 编辑菜单
    connect(m_undoAction, &QAction::triggered, this, &MainWindow::undo);
    connect(m_redoAction, &QAction::triggered, this, &MainWindow::redo);
    connect(m_cutAction, &QAction::triggered, this, &MainWindow::cut);
    connect(m_copyAction, &QAction::triggered, this, &MainWindow::copy);
    connect(m_pasteAction, &QAction::triggered, this, &MainWindow::paste);
    connect(m_selectAllAction, &QAction::triggered, this, &MainWindow::selectAll);

    // 视图菜单
    connect(m_togglePreviewAction, &QAction::triggered, this, &MainWindow::togglePreview);
    connect(m_zoomInAction, &QAction::triggered, this, &MainWindow::zoomIn);
    connect(m_zoomOutAction, &QAction::triggered, this, &MainWindow::zoomOut);
    connect(m_resetZoomAction, &QAction::triggered, this, &MainWindow::resetZoom);

    // 帮助菜单
    connect(m_aboutAction, &QAction::triggered, this, &MainWindow::about);

    // 编辑器信号连接
    connect(m_editor, &QTextEdit::textChanged, this, &MainWindow::onTextChanged);
    connect(m_editor, &QTextEdit::cursorPositionChanged, this, &MainWindow::updateStatusBar);
    connect(m_editor, &QTextEdit::cursorPositionChanged, this, &MainWindow::updateWordCount);
    connect(m_editor->document(), &QTextDocument::undoAvailable, m_undoAction, &QAction::setEnabled);
    connect(m_editor->document(), &QTextDocument::redoAvailable, m_redoAction, &QAction::setEnabled);

    // 剪贴板状态更新
    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, [this]() {
        m_pasteAction->setEnabled(!QApplication::clipboard()->text().isEmpty());
    });

    connect(m_toggleLineNumbersAction, &QAction::triggered, this, &MainWindow::toggleLineNumbers);
    connect(m_toggleWordWrapAction, &QAction::triggered, this, &MainWindow::toggleWordWrap);
    connect(m_toggleSyntaxHighlightAction, &QAction::triggered, this, &MainWindow::toggleSyntaxHighlight);
    connect(m_toggleDarkThemeAction, &QAction::triggered, this, &MainWindow::toggleDarkTheme);
    connect(m_toggleAIAction,&QAction::triggered, this, &MainWindow::toggleAIPanel);
    connect(m_findAction, &QAction::triggered, this, &MainWindow::showFindDialog);
    connect(m_replaceAction, &QAction::triggered, this, &MainWindow::replace);
}

// ==================== 文件操作槽函数 ====================

void MainWindow::newFile()
{
    if (maybeSave()) {
        if (auto *vEditor = dynamic_cast<VirtualizedEditor*>(m_editor)) {
            vEditor->setFullContent("");
        } else {
            m_editor->clear();
        }
        setCurrentFile("");
        m_isFirstPreviewRender = true; // Bug 5修复：新文件首次渲染标记重置
        m_statusLabel->setText("已创建新文件");
    }
}

void MainWindow::openFile()
{
    if (maybeSave()) {
        QString fileName = QFileDialog::getOpenFileName(
            this,
            "打开Markdown文件",
            "",
            "Markdown文件 (*.md *.markdown);;文本文件 (*.txt);;所有文件 (*.*)"
            );

        if (!fileName.isEmpty()) {
            loadFile(fileName);
        }
    }
}

void MainWindow::saveFile()
{
    if (m_currentFile.isEmpty()) {
        saveAs();
    } else {
        QFile file(m_currentFile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << editorFullContent();

            m_isModified = false;
            updateWindowTitle();
            m_statusLabel->setText("文件已保存");
        } else {
            QMessageBox::warning(this, "错误", "无法保存文件！");
        }
    }
}

void MainWindow::saveAs()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "保存Markdown文件",
        "",
        "Markdown文件 (*.md);;文本文件 (*.txt);;所有文件 (*.*)"
        );

    if (!fileName.isEmpty()) {
        setCurrentFile(fileName);
        saveFile();
    }
}

void MainWindow::showExportSuccessMsg(const QString &format, const QString &filePath)
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(QString("导出%1成功").arg(format));
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText(QString("已成功将内容导出为%1文件：\n%2").arg(format).arg(filePath));
    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec(); // 阻塞显示提示，用户点击确定后关闭
}

void MainWindow::exportToHtml()
{
    // 1. 弹出文件保存对话框，让用户选择保存路径
    QString defaultFileName = "未命名文档";
    if (!m_currentFile.isEmpty()) {
        // 若当前有打开的文件，默认用原文件名（替换后缀为.html）
        QFileInfo fileInfo(m_currentFile);
        defaultFileName = fileInfo.baseName(); // 去掉后缀的文件名（如“笔记”）
    }
    QString filePath = QFileDialog::getSaveFileName(
        this,                                  // 父窗口
        "导出为HTML文件",                       // 对话框标题
        QString("%1.html").arg(defaultFileName),// 默认文件名（含.html后缀）
        "HTML文件 (*.html);;所有文件 (*.*)"     // 文件类型过滤
        );

    // 2. 若用户取消选择路径，直接返回
    if (filePath.isEmpty()) {
        m_statusLabel->setText("已取消HTML导出");
        return;
    }

    // 3. 从预览区获取带样式的HTML内容（直接复用预览区的渲染结果）
    QString htmlContent = m_preview->toHtml();

    // 4. 写入HTML文件（处理编码，避免中文乱码）
    QFile htmlFile(filePath);
    // 关键：用UTF-8编码写入，确保中文显示正常
    if (htmlFile.open(QIODevice::WriteOnly | QIODevice::Text | QFile::Truncate)) {
        QTextStream outStream(&htmlFile);
        outStream.setEncoding(QStringConverter::Utf8); // 设置编码为UTF-8
        outStream << htmlContent;    // 写入HTML内容
        htmlFile.close();            // 关闭文件

        // 5. 提示成功并更新状态栏
        m_statusLabel->setText(QString("已导出HTML文件：%1").arg(filePath));
        showExportSuccessMsg("HTML", filePath);
    } else {
        // 6. 处理文件打开失败（如权限不足）
        QMessageBox::warning(
            this,
            "导出HTML失败",
            QString("无法写入文件：%1\n原因：%2").arg(filePath).arg(htmlFile.errorString())
            );
        m_statusLabel->setText("HTML导出失败");
    }
}

// 导出为PDF文件（核心函数）
void MainWindow::exportToPdf()
{
    // 1. 弹出文件保存对话框，让用户选择保存路径
    QString defaultFileName = "未命名文档";
    if (!m_currentFile.isEmpty()) {
        QFileInfo fileInfo(m_currentFile);
        defaultFileName = fileInfo.baseName();
    }
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "导出为PDF文件",
        QString("%1.pdf").arg(defaultFileName), // 默认后缀为.pdf
        "PDF文件 (*.pdf);;所有文件 (*.*)"
        );
    if (filePath.isEmpty()) {
        m_statusLabel->setText("已取消PDF导出");
        return;
    }

    // 2. 初始化PDF打印机（设置分辨率、页面大小、边距）
    QPrinter pdfPrinter(QPrinter::HighResolution); // 高分辨率（确保文字清晰）
    pdfPrinter.setOutputFormat(QPrinter::PdfFormat); // 输出格式为PDF
    pdfPrinter.setOutputFileName(filePath);         // 输出文件路径
    pdfPrinter.setPageSize(QPageSize::A4);         // 页面大小为A4
    pdfPrinter.setPageMargins(QMarginsF(20, 20, 20, 20), QPageLayout::Millimeter); // 页边距20mm

    // 3. 准备PDF内容（复用预览区的HTML，但适配打印样式）
    QTextDocument pdfDoc;
    QString previewHtml = m_preview->toHtml();

    // 关键：修改HTML样式，适配PDF打印（避免内容超出页面）
    QString pdfHtml = previewHtml;
    // 替换body样式：移除背景色（打印无需背景），限制宽度为A4内容宽度（约190mm）
    pdfHtml.replace(
        "<body",
        "<body style=\"background-color: transparent; max-width: 190mm; margin: 0 auto;\""
        );
    // 替换表格样式：确保表格不超出页面（自动换行）
    pdfHtml.replace(
        "table style=\"border-collapse: collapse; width: 100%;",
        "table style=\"border-collapse: collapse; width: 100%; table-layout: fixed; word-wrap: break-word;\""
        );

    // 4. 将HTML设置到文档并打印为PDF
    pdfDoc.setHtml(pdfHtml);
    pdfDoc.print(&pdfPrinter); // 核心打印操作（生成PDF）

    // 5. 验证PDF是否生成成功（检查文件是否存在且大小大于0）
    QFileInfo pdfFileInfo(filePath);
    if (pdfFileInfo.exists() && pdfFileInfo.size() > 0) {
        m_statusLabel->setText(QString("已导出PDF文件：%1").arg(filePath));
        showExportSuccessMsg("PDF", filePath);
    } else {
        QMessageBox::warning(
            this,
            "导出PDF失败",
            QString("PDF文件生成失败：%1\n可能原因：打印机模块未启用或路径无权限").arg(filePath)
            );
        m_statusLabel->setText("PDF导出失败");
    }
}
// ==================== 编辑操作槽函数 ====================

void MainWindow::undo()
{
    m_editor->undo();
    m_statusLabel->setText("撤销操作");
}

void MainWindow::redo()
{
    m_editor->redo();
    m_statusLabel->setText("重做操作");
}

void MainWindow::cut()
{
    m_editor->cut();
    m_statusLabel->setText("已剪切选中文本");
}

void MainWindow::copy()
{
    m_editor->copy();
    m_statusLabel->setText("已复制选中文本");
}

void MainWindow::paste()
{
    m_editor->paste();
    m_statusLabel->setText("已粘贴剪贴板内容");
}
void MainWindow::selectAll()
{
    m_editor->selectAll();
    m_statusLabel->setText("已全选文本");
}
// ==================== 视图操作槽函数 ====================

void MainWindow::togglePreview()
{
    m_previewVisible = !m_previewVisible;
    m_preview->setVisible(m_previewVisible);

    if (m_previewVisible) {
        m_splitter->setSizes({100, 600, 500});
        m_statusLabel->setText("预览面板已显示");
    } else {
        m_splitter->setSizes({100,900});
        m_statusLabel->setText("预览面板已隐藏");
    }
}

void MainWindow::zoomIn()
{
    m_zoomFactor += ZOOM_STEP;
    QFont font = m_editor->font();
    font.setPointSizeF(font.pointSizeF() + ZOOM_STEP);
    m_editor->setFont(font);
    m_preview->setFont(font);
    m_lineNumberArea->setFont(font);
    m_statusLabel->setText(QString("已放大到 %1%").arg(int(90+m_zoomFactor * 10)));
}

void MainWindow::zoomOut()
{
    if (m_zoomFactor > ZOOM_STEP) {
        m_zoomFactor -= ZOOM_STEP;
        QFont font = m_editor->font();
        font.setPointSizeF(font.pointSizeF() - ZOOM_STEP);
        m_editor->setFont(font);
        m_preview->setFont(font);
        m_lineNumberArea->setFont(font);
        m_statusLabel->setText(QString("已缩小到 %1%").arg(int(90+m_zoomFactor * 10)));
    }
}

void MainWindow::resetZoom()
{
    m_zoomFactor = ZOOM_DEFAULT;
    QFont font = m_editor->font();
    font.setPointSize(11);  // 重置为默认大小
    m_editor->setFont(font);
    m_preview->setFont(font);
    m_lineNumberArea->setFont(font);
    m_statusLabel->setText("缩放已重置");
}

// ==================== 其他槽函数 ====================

void MainWindow::about()
{
    QMessageBox::about(this, "关于 Markdown编辑器",
                       "<h2>Markdown编辑器</h2>"
                       "<li>完整的Markdown语法支持</li>"
                       "<li>专业的HTML预览</li>"
                       "<li>撤销/重做功能</li>"
                       "<li>文本缩放控制</li>"
                       "<li>预览面板切换</li>"
                       "<li>增强的状态栏</li>"
                       "<li>工具栏快捷操作</li>"
                       "</ul>"
                       "<p>使用 Qt 和 C++ 开发</p>");
}

void MainWindow::updatePreview()
{ 
    if (m_chatPanel) {
        m_chatPanel->setDocumentContent(editorFullContent());
    }

    QString markdownText = editorFullContent();
    QString html;

    QSet<int> changedLinesSet = m_diffRenderer->getChangedLines();
    QList<int> changedLines = changedLinesSet.values();
    std::sort(changedLines.begin(), changedLines.end());

    // Bug 5修复：用成员变量替代static，打开新文件时正确重置
    bool needFullRender = m_isFirstPreviewRender || !m_diffRenderEnabled;

    html = MarkdownParser::toHtml(markdownText);
    m_isFirstPreviewRender = false;

    qDebug() << (needFullRender ? "全量渲染（首次或差分已关闭）" : "差分渲染（保持滚动位置）");

    // ---------------------- 主题样式优化（标题间距+行高） ----------------------
    QString themeCss;
    if (m_darkThemeEnabled) {
        themeCss = R"(
            body { font-family: 'Segoe UI', Arial, sans-serif; line-height: 1.5; color: #ecf0f1; background-color: #2c3e50; margin: 10px; }
            h1, h2, h3 { color: #81d4fa; margin-top: 1em; margin-bottom: 0.5em; padding-bottom: 0.3em; border-bottom: 1px solid #34495e; }
            h1 { font-size: 1.8em; }
            h2 { font-size: 1.4em; }
            h3 { font-size: 1.2em; }
            code { background: #34495e; color: #ef9a9a; padding: 2px 6px; border-radius: 3px; font-family: 'Consolas', monospace; }
            pre { background: #2c3e50; color: #ecf0f1; padding: 10px; border-radius: 5px; overflow-x: auto; border-left: 4px solid #42a5f5; }
            a { color: #42a5f5; text-decoration: none; }
            a:hover { text-decoration: underline; }
            ul, ol { margin: 0.5em 0; padding-left: 2em; color: #ecf0f1; }
            li { margin: 0.3em 0; }
            blockquote { border-left: 4px solid #34495e; margin: 0.5em 0; padding-left: 10px; color: #bdc3c7; }
            img { max-width: 100%; height: auto; border-radius: 5px; }
            table { border-collapse: collapse; width: 100%; margin: 0.5em 0; }
            th { border: 1px solid #34495e; padding: 6px; text-align: left; background-color: #34495e; color: #ecf0f1; }
            td { border: 1px solid #34495e; padding: 6px; text-align: left; color: #ecf0f1; }
        )";
    } else {
        themeCss = R"(
            body { font-family: 'Segoe UI', Arial, sans-serif; line-height: 1.5; color: #333; margin: 10px; }
            h1, h2, h3 { color: #2c3e50; margin-top: 1em; margin-bottom: 0.5em; padding-bottom: 0.3em; border-bottom: 1px solid #eee; }
            h1 { font-size: 1.8em; }
            h2 { font-size: 1.4em; }
            h3 { font-size: 1.2em; }
            code { background: #f4f4f4; padding: 2px 6px; border-radius: 3px; font-family: 'Consolas', monospace; }
            pre { background: #f8f9fa; padding: 10px; border-radius: 5px; overflow-x: auto; border-left: 4px solid #007acc; }
            a { color: #007acc; text-decoration: none; }
            a:hover { text-decoration: underline; }
            ul, ol { margin: 0.5em 0; padding-left: 2em; }
            li { margin: 0.3em 0; }
            blockquote { border-left: 4px solid #ddd; margin: 0.5em 0; padding-left: 10px; color: #666; }
            img { max-width: 100%; height: auto; border-radius: 5px; }
            table { border-collapse: collapse; width: 100%; margin: 0.5em 0; }
            th { border: 1px solid #ddd; padding: 6px; text-align: left; background-color: #f5f5f5; }
            td { border: 1px solid #ddd; padding: 6px; text-align: left; }
        )";
    }

    QString styledHtml = QString(
                             "<html>"
                             "<head>"
                             "<meta charset=\"UTF-8\">"
                             "<style>%1</style>"
                             "</head>"
                             "<body>%2</body>"
                             "</html>"
                             ).arg(themeCss, html);

    if (needFullRender) {
        // 全量渲染时强制滚动条置顶
        m_preview->verticalScrollBar()->setValue(0);
        m_preview->setHtml(styledHtml);
        m_lastPreviewHtml = styledHtml;
    } else {
        // 差分渲染时恢复用户滚动位置
        QTextCursor cursor = m_preview->textCursor();
        int scrollPos = m_preview->verticalScrollBar()->value();

        m_preview->setHtml(styledHtml);

        m_preview->setTextCursor(cursor);
        m_preview->verticalScrollBar()->setValue(scrollPos);
    }

    m_diffRenderer->resetChangedLines();
}

void MainWindow::updateStatusBar()
{
    // 更新光标位置
    QTextCursor cursor = m_editor->textCursor();//获取光标信息
    int line = cursor.blockNumber() + 1;//blocknumber返回索引从0开始
    int column = cursor.columnNumber() + 1;
    m_cursorPositionLabel->setText(QString("行: %1, 列: %2").arg(line).arg(column));

    // 更新文件信息
    m_fileInfoLabel->setText(getFileDescription());

    // 更新编辑动作状态
    m_cutAction->setEnabled(cursor.hasSelection());
    m_copyAction->setEnabled(cursor.hasSelection());
}

void MainWindow::autoSave()
{
    // 若自动保存被禁用，直接返回
    if (!m_autoSaveEnabled) {
        return;
    }

    // 情况1：当前文件已命名（有m_currentFile）
    if (m_isModified && !m_currentFile.isEmpty()) {
        QFile file(m_currentFile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text | QFile::Truncate)) {
            QTextStream out(&file);
            out.setEncoding(QStringConverter::Utf8); // 用UTF-8编码保存，避免中文乱码
            out << editorFullContent();
            file.close();
            m_statusLabel->setText(QString("已自动保存：%1").arg(QFileInfo(m_currentFile).fileName()));
        } else {
            // 保存失败：提示用户（如文件被占用）
            QString errorMsg = QString("自动保存失败：%1\n原因：%2").arg(m_currentFile).arg(file.errorString());
            m_statusLabel->setText(errorMsg);
            // 弹出警告对话框（非阻塞，避免打断用户操作）
            QTimer::singleShot(0, this, [this, errorMsg]() {
                QMessageBox::warning(this, "自动保存警告", errorMsg);
            });
        }
    }
    // 情况2：当前文件未命名（m_currentFile为空）→ 保存到临时目录
    else if (m_isModified && m_currentFile.isEmpty()) {
        // 临时文件名：基于当前时间（避免重复），如“AutoSave_20251015_143025.md”
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss");
        QString tempFileName = QString("AutoSave_%1.md").arg(timestamp);
        QString tempFilePath = m_autoSaveTempPath + QDir::separator() + tempFileName;

        QFile tempFile(tempFilePath);
        if (tempFile.open(QIODevice::WriteOnly | QIODevice::Text | QFile::Truncate)) {
            QTextStream out(&tempFile);
            out.setEncoding(QStringConverter::Utf8);
            out << editorFullContent();
            tempFile.close();
            m_statusLabel->setText(QString("已自动保存到临时文件：%1").arg(tempFileName));
            // 清理3天前的旧临时文件（避免占用空间）
            cleanOldAutoSaveFiles();
        } else {
            QString errorMsg = QString("临时文件自动保存失败：%1\n原因：%2").arg(tempFilePath).arg(tempFile.errorString());
            m_statusLabel->setText(errorMsg);
            QTimer::singleShot(0, this, [this, errorMsg]() {
                QMessageBox::warning(this, "自动保存警告", errorMsg);
            });
        }
    }
}

// 新增：清理3天前的旧自动保存临时文件
void MainWindow::cleanOldAutoSaveFiles()
{
    QDir tempDir(m_autoSaveTempPath);
    if (!tempDir.exists()) return;

    // 筛选.md格式的自动保存文件
    QStringList filters;
    filters << "AutoSave_*.md";
    tempDir.setNameFilters(filters);
    QFileInfoList fileList = tempDir.entryInfoList(QDir::Files, QDir::Time); // 按时间排序

    // 保留最近3天的文件，删除更早的
    const qint64 threeDaysMs = 3 * 24 * 60 * 60 * 1000; // 3天的毫秒数
    QDateTime now = QDateTime::currentDateTime();

    foreach (const QFileInfo &fileInfo, fileList) {
        qint64 timeDiff = fileInfo.lastModified().msecsTo(now);
        if (timeDiff > threeDaysMs) {
            tempDir.remove(fileInfo.fileName()); // 删除旧文件
            qDebug() << "已清理旧自动保存文件：" << fileInfo.fileName();
        }
    }
}

void MainWindow::toggleLineNumbers()
{
    m_lineNumbersVisible = !m_lineNumbersVisible;
    m_lineNumberArea->setVisible(m_lineNumbersVisible);

    if (m_lineNumbersVisible) {
        m_statusLabel->setText("行号显示已开启");
        updateLineNumbers();
    } else {
        m_statusLabel->setText("行号显示已关闭");
    }
}

void MainWindow::toggleWordWrap()
{
    m_wordWrapEnabled = !m_wordWrapEnabled;

    if (m_wordWrapEnabled) {
        m_editor->setLineWrapMode(QTextEdit::WidgetWidth);
        m_statusLabel->setText("自动换行已开启");
    } else {
        m_editor->setLineWrapMode(QTextEdit::NoWrap);
        m_statusLabel->setText("自动换行已关闭");
    }
}

void MainWindow::toggleSyntaxHighlight()
{
    m_syntaxHighlightEnabled = !m_syntaxHighlightEnabled;

    // Bug 3修复：仅在编辑区操作高亮器（预览区由CSS控制）
    if (m_syntaxHighlightEnabled) {
        if (m_editorHighlighter && m_editor->document()) {
            m_editorHighlighter->setDocument(m_editor->document());
            m_editorHighlighter->rehighlight();
        }
        m_statusLabel->setText("语法高亮已开启");
    } else {
        if (m_editorHighlighter) {
            m_editorHighlighter->setDocument(nullptr);
        }
        m_statusLabel->setText("语法高亮已关闭");
    }
    // 强制更新预览以确保样式生效
    updatePreview();
}

void MainWindow::toggleDarkTheme()
{
    m_darkThemeEnabled = !m_darkThemeEnabled;

    // 更新编辑器样式（编辑区保持简单样式，无语法高亮）
    if (m_darkThemeEnabled) {
        // 编辑区：暗色主题
        m_editor->setStyleSheet(
            "QTextEdit {"
            "   background-color: #2b2b2b;"
            "   color: #a9b7c6;"
            "   selection-background-color: #214283;"
            "   border: 1px solid #555;"
            "}"
            );
        m_lineNumberArea->setStyleSheet(
            "QTextEdit {"
            "   background-color: #1e1e1e;"
            "   color: #858585;"
            "   border: none;"
            "}"
            );
    } else {
        // 编辑区：亮色主题
        m_editor->setStyleSheet(
            "QTextEdit {"
            "   background-color: #ffffff;"
            "   color: #000000;"
            "   border: 1px solid #ccc;"
            "}"
            );
        m_lineNumberArea->setStyleSheet(
            "QTextEdit {"
            "   background-color: #f5f5f5;"
            "   color: #666;"
            "   border: none;"
            "}"
            );
    }

    // Bug 3修复：只更新编辑区高亮器主题
    if (m_editorHighlighter) {
        m_editorHighlighter->setTheme(m_darkThemeEnabled ? "dark" : "light");
    }
    // 更新预览内容
    updatePreview();
    m_statusLabel->setText(m_darkThemeEnabled ? "暗色主题已启用" : "亮色主题已启用");
}

void MainWindow::updateLineNumbers()
{
    if (!m_lineNumbersVisible || !m_lineNumberArea || !m_editor) return;

    QTextDocument *doc = m_editor->document();
    int lineCount = doc->blockCount();

    // ---------------------- 修复1：行号文本与编辑区行高对齐 ----------------------
    QString numbers;
    for (int i = 1; i <= lineCount; ++i) {
        // 每个行号后添加换行，确保行号区域的行数与编辑区一致
        numbers += QString::number(i) + "\n";
    }
    // 移除最后一个多余的换行
    if (!numbers.isEmpty()) {
        numbers.chop(1);
    }
    m_lineNumberArea->setPlainText(numbers);

    // ---------------------- 修复2：滚动位置精确同步 ----------------------
    QScrollBar *editorScroll = m_editor->verticalScrollBar();
    QScrollBar *lineScroll = m_lineNumberArea->verticalScrollBar();
    if (editorScroll && lineScroll) {
        // 关键：同步滚动值（确保行号与编辑区顶部对齐）
        lineScroll->setValue(editorScroll->value());
        // 同步滚动步长（确保滚动速度一致）
        lineScroll->setSingleStep(editorScroll->singleStep());
        lineScroll->setPageStep(editorScroll->pageStep());
        // 同步滚动范围（避免行号区域滚动溢出）
        lineScroll->setRange(editorScroll->minimum(), editorScroll->maximum());
    }

    // ---------------------- 修复3：行号区域宽度自适应 ----------------------
    int digits = QString::number(lineCount).length();
    // 计算行号区域宽度（基于最大行号的长度 + 边距）
    QFontMetrics fm(m_lineNumberArea->font());
    int width = fm.horizontalAdvance(QString("9").repeated(qMax(3, digits))) + 30; // +30覆盖padding+border
    m_lineNumberArea->setFixedWidth(width);

    // ---------------------- 修复4：行号区域字体与编辑区完全一致 ----------------------
    m_lineNumberArea->setFont(m_editor->font());
}

void MainWindow::updateWordCount()
{
    QString text = editorFullContent();
    m_charCount = text.length(); // 字符数：直接取文本长度（含空格、换行）

    // 优化：字数统计逻辑（中文按字符算1个词，英文按空格分割算词，排除空字符）
    m_wordCount = 0;
    const QChar *textData = text.constData(); // 文本的字符数组指针
    int textLen = text.length();
    bool inEnglishWord = false; // 是否处于英文单词中

    for (int i = 0; i < textLen; ++i) {
        QChar c = textData[i];
        // 情况1：中文/日文/韩文等东亚字符（Unicode范围：U+4E00-U+9FFF，U+3040-U+30FF，U+AC00-U+D7AF）
        if ((c.unicode() >= 0x4E00 && c.unicode() <= 0x9FFF) ||
            (c.unicode() >= 0x3040 && c.unicode() <= 0x30FF) ||
            (c.unicode() >= 0xAC00 && c.unicode() <= 0xD7AF)) {
            m_wordCount++; // 每个东亚字符算1个词
            inEnglishWord = false; // 退出英文单词状态
        }
        // 情况2：英文单词字符（字母、数字、下划线，符合单词定义）
        else if (c.isLetterOrNumber() || c == '_') {
            if (!inEnglishWord) {
                m_wordCount++; // 新单词的开始，字数+1
                inEnglishWord = true; // 进入英文单词状态
            }
            // 若已在单词中，不重复计数
        }
        // 情况3：分隔符（空格、换行、标点等）→ 退出英文单词状态
        else {
            inEnglishWord = false;
        }
    }

    // 更新状态栏显示（优化：区分“字数”和“字符数”的说明）
    m_wordCountLabel->setText(QString("字数：%1 | 字符数：%2").arg(m_wordCount).arg(m_charCount));
}

void MainWindow::find()
{
    QString findText = m_findEdit->text();
    if (findText.isEmpty()) return;

    // Bug 16修复：在全文上搜索（虚拟化编辑器可见区只是窗口）
    VirtualizedEditor *vEditor = dynamic_cast<VirtualizedEditor*>(m_editor);
    QString fullText = vEditor ? vEditor->getFullContent() : m_editor->toPlainText();

    QTextCursor cursor = m_editor->textCursor();
    int cursorPos = vEditor
        ? vEditor->getVisibleStartLine() * 80 + cursor.position() // 近似全局位置
        : cursor.position();

    int position = fullText.indexOf(findText, cursorPos);
    if (position != -1) {
        // 在全文找到 → 需要滚动VirtualizedEditor到目标位置
        if (vEditor) {
            QString beforeMatch = fullText.left(position);
            int targetLine = beforeMatch.count('\n');
            int targetCol = position - beforeMatch.lastIndexOf('\n') - 1;
            // 简化处理：直接用toPlainText在可见区内查找
            // 若不在可见区，回退到可见区查找
            QTextCursor visCursor = m_editor->textCursor();
            visCursor.setPosition(qMin(position, m_editor->toPlainText().length()));
            visCursor.setPosition(qMin(position + findText.length(), m_editor->toPlainText().length()), QTextCursor::KeepAnchor);
            m_editor->setTextCursor(visCursor);
            m_statusLabel->setText("已找到文本（可能在不可见区域）");
        } else {
            cursor.setPosition(position);
            cursor.setPosition(position + findText.length(), QTextCursor::KeepAnchor);
            m_editor->setTextCursor(cursor);
            m_statusLabel->setText("已找到文本");
        }
    } else {
        m_statusLabel->setText("未找到文本");
    }
}

void MainWindow::replace()
{
    showFindDialog();  // 简化实现，显示查找替换对话框
}

void MainWindow::showFindDialog()
{
    // 创建简单的查找替换对话框
    QDialog *findDialog = new QDialog(this);
    findDialog->setWindowTitle("查找和替换");
    findDialog->setModal(false);
    findDialog->resize(400, 200);

    QVBoxLayout *layout = new QVBoxLayout(findDialog);

    // 查找部分
    QGroupBox *findGroup = new QGroupBox("查找", findDialog);
    QHBoxLayout *findLayout = new QHBoxLayout(findGroup);

    QLabel *findLabel = new QLabel("查找:", findGroup);
    m_findEdit = new QLineEdit(findGroup);
    QPushButton *findButton = new QPushButton("查找下一个", findGroup);

    findLayout->addWidget(findLabel);
    findLayout->addWidget(m_findEdit);
    findLayout->addWidget(findButton);

    // 替换部分
    QGroupBox *replaceGroup = new QGroupBox("替换", findDialog);
    QHBoxLayout *replaceLayout = new QHBoxLayout(replaceGroup);

    QLabel *replaceLabel = new QLabel("替换为:", replaceGroup);
    m_replaceEdit = new QLineEdit(replaceGroup);
    QPushButton *replaceButton = new QPushButton("替换", replaceGroup);
    QPushButton *replaceAllButton = new QPushButton("全部替换", replaceGroup);

    replaceLayout->addWidget(replaceLabel);
    replaceLayout->addWidget(m_replaceEdit);
    replaceLayout->addWidget(replaceButton);
    replaceLayout->addWidget(replaceAllButton);

    // 选项部分
    QGroupBox *optionGroup = new QGroupBox("选项", findDialog);
    QHBoxLayout *optionLayout = new QHBoxLayout(optionGroup);

    QCheckBox *caseSensitiveCheck = new QCheckBox("区分大小写", optionGroup);
    QCheckBox *wholeWordCheck = new QCheckBox("全字匹配", optionGroup);

    optionLayout->addWidget(caseSensitiveCheck);
    optionLayout->addWidget(wholeWordCheck);

    // 按钮部分
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *closeButton = new QPushButton("关闭", findDialog);

    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);

    // 组合所有部分
    layout->addWidget(findGroup);
    layout->addWidget(replaceGroup);
    layout->addWidget(optionGroup);
    layout->addLayout(buttonLayout);

    // 连接信号
    connect(findButton, &QPushButton::clicked, this, [this, caseSensitiveCheck, wholeWordCheck]() {
        findText(m_findEdit->text(), caseSensitiveCheck->isChecked(), wholeWordCheck->isChecked());
    });

    connect(replaceButton, &QPushButton::clicked, this, [this, caseSensitiveCheck, wholeWordCheck]() {
        replaceText(m_findEdit->text(), m_replaceEdit->text(),
                    caseSensitiveCheck->isChecked(), wholeWordCheck->isChecked());
    });

    connect(replaceAllButton, &QPushButton::clicked, this, [this, caseSensitiveCheck, wholeWordCheck]() {
        replaceAll(m_findEdit->text(), m_replaceEdit->text(),
                   caseSensitiveCheck->isChecked(), wholeWordCheck->isChecked());
    });

    connect(closeButton, &QPushButton::clicked, findDialog, &QDialog::close);

    findDialog->show();
}

void MainWindow::findText(const QString &text, bool caseSensitive, bool wholeWord)
{
    if (text.isEmpty()) return;

    QTextDocument::FindFlags flags;
    if (caseSensitive) {
        flags |= QTextDocument::FindCaseSensitively;
    }
    if (wholeWord) {
        flags |= QTextDocument::FindWholeWords;
    }

    QTextCursor cursor = m_editor->textCursor();
    cursor = m_editor->document()->find(text, cursor, flags);

    if (!cursor.isNull()) {
        m_editor->setTextCursor(cursor);
        m_statusLabel->setText("已找到文本");
    } else {
        // 从文档开始重新查找
        cursor = m_editor->textCursor();
        cursor.setPosition(0);
        cursor = m_editor->document()->find(text, cursor, flags);

        if (!cursor.isNull()) {
            m_editor->setTextCursor(cursor);
            m_statusLabel->setText("已找到文本（从开头开始）");
        } else {
            m_statusLabel->setText("未找到文本");
        }
    }
}

void MainWindow::replaceText(const QString &findText, const QString &replaceText,
                             bool caseSensitive, bool wholeWord)
{
    QTextCursor cursor = m_editor->textCursor();
    if (cursor.hasSelection() && cursor.selectedText() == findText) {
        cursor.insertText(replaceText);
        m_statusLabel->setText("已替换文本");
    } else {
        // ========== 修复这里 ==========
        // 调用 m_editor 的 find 方法来查找文本
        bool found = m_editor->find(findText, QTextDocument::FindFlags(0)
                                                  | (caseSensitive ? QTextDocument::FindCaseSensitively : QTextDocument::FindFlags())
                                                  | (wholeWord ? QTextDocument::FindWholeWords : QTextDocument::FindFlags()));
        if (found) {
            // 查找成功后，再执行替换（可选：比如选中后调用 insertText）
            cursor = m_editor->textCursor();
            cursor.insertText(replaceText);
            m_statusLabel->setText("已查找并替换文本");
        } else {
            m_statusLabel->setText("未找到要替换的文本");
        }
        // ========== 修复结束 ==========
    }
}

void MainWindow::replaceAll(const QString &findText, const QString &replaceText,
                            bool caseSensitive, bool wholeWord)
{
    // Bug 16修复：虚拟化编辑器在全文上做替换
    VirtualizedEditor *vEditor = dynamic_cast<VirtualizedEditor*>(m_editor);
    if (vEditor) {
        QString fullContent = vEditor->getFullContent();
        Qt::CaseSensitivity cs = caseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive;
        int count = 0;
        int pos = 0;
        while ((pos = fullContent.indexOf(findText, pos, cs)) != -1) {
            fullContent.replace(pos, findText.length(), replaceText);
            pos += replaceText.length();
            count++;
        }
        vEditor->setFullContent(fullContent);
        m_statusLabel->setText(QString("已替换 %1 处").arg(count));
        return;
    }

    QTextDocument::FindFlags flags;
    if (caseSensitive) flags |= QTextDocument::FindCaseSensitively;
    if (wholeWord) flags |= QTextDocument::FindWholeWords;

    QTextCursor cursor(m_editor->document());
    cursor.setPosition(0);

    int replaceCount = 0;
    while (true) {
        cursor = m_editor->document()->find(findText, cursor, flags);
        if (cursor.isNull()) break;
        cursor.insertText(replaceText);
        replaceCount++;
    }

    m_statusLabel->setText(QString("已替换 %1 处").arg(replaceCount));
}

// Bug 10修复：删除手写detectFileEncoding()，统一使用FileEncodingDetector类

void MainWindow::applyEditorSettings()
{
    // 应用所有编辑器设置
    toggleLineNumbers();
    toggleWordWrap();
    toggleSyntaxHighlight();
    toggleDarkTheme();
}
void MainWindow::onTextChanged()
{
    // 1. 立即执行轻量操作（不影响性能）
    m_isModified = true;
    updateWindowTitle(); // 窗口标题标记“已修改”（用户即时感知）

    // 2. 重启延迟定时器（核心优化：合并快速输入的多次触发）
    if (m_updateTimer->isActive()) {
        m_updateTimer->stop(); // 重置定时器
    }
    m_updateTimer->start(); // 重新计时

    // 3. Bug 18修复：用成员变量 m_autoSaveDebounce 替代 static QTimer
    if (m_autoSaveEnabled) {
        if (m_autoSaveDebounce->isActive()) {
            m_autoSaveDebounce->stop();
        }
        m_autoSaveDebounce->start();
    }

    // 4. 行号实时更新（轻量操作，立即执行）
    updateLineNumbers();
}

void MainWindow::updateWindowTitle()
{
    QString title = "Markdown编辑器";

    if (!m_currentFile.isEmpty()) {
        QFileInfo fileInfo(m_currentFile);
        title += " - " + fileInfo.fileName();
    } else {
        title += " - 新文件";
    }

    if (m_isModified) {
        title += " [已修改]";
    }

    setWindowTitle(title);
}

// ==================== 业务逻辑方法 ====================

bool MainWindow::maybeSave()
{
    if (!m_isModified) {
        return true;
    }

    QMessageBox::StandardButton ret = QMessageBox::warning(
        this,
        "Markdown编辑器",
        "文档已被修改，是否保存更改？",
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
        );

    if (ret == QMessageBox::Save) {
        saveFile();
        return true;
    } else if (ret == QMessageBox::Cancel) {
        return false;
    }

    return true;
}

bool MainWindow::loadFile(const QString &fileName)
{
    // 1. 检查文件是否存在
    QFile file(fileName);
    if (!file.exists()) {
        QMessageBox::warning(this, "错误", QString("文件不存在：%1").arg(fileName));
        return false;
    }

    // 2. 尝试打开文件（增加详细错误信息）
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(
            this,
            "打开失败",
            QString("无法打开文件：%1\n原因：%2").arg(fileName).arg(file.errorString())
            );
        return false;
    }

    // 3. 检测文件大小，决定加载方式（大文件分段加载）
    qint64 fileSize = file.size();
    const qint64 LARGE_FILE_THRESHOLD = 1024 * 1024; // 1MB以上视为大文件
    QByteArray data;

    if (fileSize <= LARGE_FILE_THRESHOLD) {
        // 小文件：一次性读取
        data = file.readAll();
    } else {
        // 大文件：分段读取（避免内存占用过高）
        data.reserve(fileSize); // 预分配内存
        const qint64 BLOCK_SIZE = 1024 * 512; // 每次读取512KB
        while (!file.atEnd()) {
            data += file.read(BLOCK_SIZE);
            // 可添加进度提示（可选）
            int progress = (file.pos() * 100) / fileSize;
            m_statusLabel->setText(QString("正在加载... %1%").arg(progress));
        }
    }
    file.close();

    // 4. Bug 6/7/10修复：使用 FileEncodingDetector（完整支持UTF-8/16/GBK）
    FileEncodingDetector detector;
    QString encoding = detector.detectFileEncoding(fileName);
    QString content;

    if (encoding == "UTF-8 with BOM" || (encoding == "UTF-8" && data.startsWith("\xEF\xBB\xBF"))) {
        content = QString::fromUtf8(data.constData() + 3, data.size() - 3);
    } else if (encoding == "UTF-8") {
        content = QString::fromUtf8(data);
    } else if (encoding == "UTF-16LE") {
        content = QString::fromUtf16(reinterpret_cast<const char16_t*>(data.constData() + 2), (data.size() - 2) / 2);
    } else if (encoding == "UTF-16BE") {
        content = QString::fromUtf16(reinterpret_cast<const char16_t*>(data.constData() + 2), (data.size() - 2) / 2);
    } else if (encoding == "GBK") {
        content = QString::fromLocal8Bit(data);
    } else {
        content = QString::fromUtf8(data);
        if (content.isEmpty() || content.contains(QChar::ReplacementCharacter)) {
            content = QString::fromLocal8Bit(data);
        }
        m_statusLabel->setText("警告：未识别编码，已尝试系统默认编码");
    }

    // 5. 适配虚拟化编辑器（如果使用了VirtualizedEditor）
    if (dynamic_cast<VirtualizedEditor*>(m_editor)) {
        // 虚拟化编辑器：使用setFullContent加载完整内容（内部只渲染可见区域）
        dynamic_cast<VirtualizedEditor*>(m_editor)->setFullContent(content);
    } else {
        // 普通编辑器：直接设置内容
        m_editor->setPlainText(content);
    }

    // 6. 更新文件状态与UI
    setCurrentFile(fileName);
    m_isFirstPreviewRender = true; // Bug 5修复：打开新文件时重置渲染标记
    m_statusLabel->setText(QString("已打开文件: %1 [编码: %2, 大小: %3KB]")
                               .arg(QFileInfo(fileName).fileName())
                               .arg(encoding)
                               .arg(fileSize / 1024));

    return true; // 加载成功
}

void MainWindow::checkAndPromptAutoSaveRecovery()
{
    // 扫描临时目录中的自动保存文件
    QStringList autoSaveFiles = m_appSettings->scanAutoSaveFiles(m_autoSaveTempPath);
    if (autoSaveFiles.isEmpty()) return;

    // 弹出恢复提示对话框
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("发现未保存内容");
    msgBox.setIcon(QMessageBox::Information);
    msgBox.setText("检测到上次退出时未保存的内容，是否恢复？");
    msgBox.setInformativeText(QString("找到%1个自动保存文件，最新的是：\n%2")
                                  .arg(autoSaveFiles.size())
                                  .arg(QFileInfo(autoSaveFiles.first()).fileName()));
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::Yes);

    if (msgBox.exec() == QMessageBox::Yes) {
        // 恢复最新的临时文件
        loadFile(autoSaveFiles.first());
        m_statusLabel->setText("已恢复上次未保存的内容");
        // 恢复后删除临时文件（避免重复提示）
        QFile::remove(autoSaveFiles.first());
    } else {
        // 用户选择不恢复，删除所有临时文件
        for (const QString &filePath : autoSaveFiles) {
            QFile::remove(filePath);
        }
        m_statusLabel->setText("已忽略未保存的临时内容");
    }
}

void MainWindow::setCurrentFile(const QString &fileName)
{
    m_currentFile = fileName;
    m_isModified = false;
    updateWindowTitle();
    updateStatusBar();
}

QString MainWindow::getFileDescription() const
{
    if (m_currentFile.isEmpty()) {
        return "新文件";
    }

    QFileInfo fileInfo(m_currentFile);
    QString size;

    qint64 fileSize = fileInfo.size();
    if (fileSize < 1024) {
        size = QString("%1 B").arg(fileSize);
    } else if (fileSize < 1024 * 1024) {
        size = QString("%1 KB").arg(fileSize / 1024.0, 0, 'f', 1);
    } else {
        size = QString("%1 MB").arg(fileSize / (1024.0 * 1024.0), 0, 'f', 1);
    }

    return QString("%1 | %2").arg(fileInfo.suffix().toUpper()).arg(size);
}

QString MainWindow::editorFullContent() const
{
    if (auto *ve = dynamic_cast<VirtualizedEditor*>(m_editor))
        return ve->getFullContent();
    return m_editor->toPlainText();
}
