#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QSplitter;
class QTextEdit;
class QMenu;
class QAction;
class QLabel;
class QTimer;

class EditorHighlighter;
class FileEncodingDetector;
class AppSettings;
class DiffRenderer;
class VirtualizedEditor;

class ChatPanel;
class DeepSeekClient;

class QLineEdit;
class QCheckBox;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void newFile();
    void openFile();
    void saveFile();
    void saveAs();
    void autoSave();
    void exportToHtml();
    void exportToPdf();
    void showExportSuccessMsg(const QString &format, const QString &filePath);

    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void selectAll();
    void find();
    void replace();

    void togglePreview();
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void toggleLineNumbers();
    void toggleWordWrap();
    void toggleSyntaxHighlight();
    void toggleDarkTheme();

    void about();
    void updatePreview();
    void updateStatusBar();
    void onTextChanged();
    void updateWindowTitle();
    void updateLineNumbers();
    void updateWordCount();
    void showFindDialog();

    void toggleAIPanel();

private:
    void setupUI();
    void setupAIPanel();
    void setupMenu();
    void setupToolBar();
    void setupStatusBar();
    void setupConnections();
    void setupSyntaxHighlighter();
    void setupLineNumbers();
    void cleanOldAutoSaveFiles();

    bool maybeSave();
    bool loadFile(const QString &fileName);
    void setCurrentFile(const QString &fileName);
    QString getFileDescription() const;
    void applyEditorSettings();
    QString editorFullContent() const; // 安全获取全文（虚编走m_fullContent，普通走toPlainText）
    AppSettings *m_appSettings;
    void checkAndPromptAutoSaveRecovery();

    void findText(const QString &text, bool caseSensitive = false, bool wholeWord = false);
    void replaceText(const QString &findText, const QString &replaceText,
                     bool caseSensitive = false, bool wholeWord = false);
    void replaceAll(const QString &findText, const QString &replaceText,
                    bool caseSensitive = false, bool wholeWord = false);

    // UI组件
    QSplitter *m_verticalSplitter;
    QSplitter *m_splitter;
    QTextEdit *m_editor;
    QTextEdit *m_preview;
    QTextEdit *m_lineNumberArea;
    QLineEdit *m_findEdit;
    QLineEdit *m_replaceEdit;

    // 状态栏
    QLabel *m_statusLabel;
    QLabel *m_cursorPositionLabel;
    QLabel *m_fileInfoLabel;
    QLabel *m_wordCountLabel;

    // 菜单和工具栏
    QMenu *m_fileMenu;
    QMenu *m_editMenu;
    QMenu *m_viewMenu;
    QMenu *m_helpMenu;
    QToolBar *m_toolBar;

    // Actions — 全部成员变量（修复Bug 11：硬编码索引）
    QAction *m_newAction;
    QAction *m_openAction;
    QAction *m_saveAction;
    QAction *m_saveAsAction;
    QAction *m_exportHtmlAction;
    QAction *m_exportPdfAction;
    QAction *m_exitAction;
    QAction *m_undoAction;
    QAction *m_redoAction;
    QAction *m_cutAction;
    QAction *m_copyAction;
    QAction *m_pasteAction;
    QAction *m_findAction;
    QAction *m_replaceAction;
    QAction *m_selectAllAction;
    QAction *m_togglePreviewAction;
    QAction *m_zoomInAction;
    QAction *m_zoomOutAction;
    QAction *m_resetZoomAction;
    QAction *m_toggleLineNumbersAction;
    QAction *m_toggleWordWrapAction;
    QAction *m_toggleSyntaxHighlightAction;
    QAction *m_toggleDarkThemeAction;
    QAction *m_aboutAction;

    // 功能组件 — 只保留编辑区高亮器（预览区用CSS）
    EditorHighlighter *m_editorHighlighter;
    QTimer *m_autoSaveTimer;
    QTimer *m_autoSaveDebounce; // Bug 18修复：成员变量替代static QTimer
    QString m_autoSaveTempPath;
    bool m_autoSaveEnabled;

    // 业务数据
    QString m_currentFile;
    bool m_isModified;
    double m_zoomFactor;
    bool m_previewVisible;
    bool m_lineNumbersVisible;
    bool m_wordWrapEnabled;
    bool m_syntaxHighlightEnabled;
    bool m_darkThemeEnabled;
    int m_wordCount;
    int m_charCount;
    bool m_isFirstPreviewRender = true; // Bug 5修复：static→成员变量

    // 差分渲染
    DiffRenderer *m_diffRenderer;
    bool m_diffRenderEnabled = true;
    QString m_lastPreviewHtml;
    QTimer *m_updateTimer;

    ChatPanel      *m_chatPanel = nullptr;
    DeepSeekClient *m_aiClient  = nullptr;
    QAction        *m_toggleAIAction = nullptr;
    bool            m_aiPanelVisible = true;
};

#endif // MAINWINDOW_H
