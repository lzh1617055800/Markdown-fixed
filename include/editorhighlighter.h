#ifndef EDITORHIGHLIGHTER_H
#define EDITORHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

// 编辑区Markdown语法高亮器（仅作用于编辑区m_editor）
class EditorHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT
public:
    explicit EditorHighlighter(QTextDocument *parent = nullptr);
    // 支持切换主题（亮色/暗色）
    void setTheme(const QString &theme);

protected:
    // 核心高亮函数：每一行文本都会调用此函数进行染色
    void highlightBlock(const QString &text) override;

private:
    // 定义不同语法元素的样式（格式）
    QTextCharFormat m_headingFormat;   // 标题样式
    QTextCharFormat m_boldFormat;      // 粗体样式
    QTextCharFormat m_italicFormat;    // 斜体样式
    QTextCharFormat m_inlineCodeFormat;// 行内代码样式
    QTextCharFormat m_blockCodeFormat; // 代码块样式
    QTextCharFormat m_linkFormat;      // 链接样式
    QTextCharFormat m_listFormat;      // 列表样式
    QTextCharFormat m_quoteFormat;     // 引用样式

    // 初始化亮色主题样式
    void setupLightTheme();
    // 初始化暗色主题样式
    void setupDarkTheme();

    bool m_inCodeBlock = false; // 每个实例独立的代码块状态（修复Bug 4：static多文档污染）
};

#endif // EDITORHIGHLIGHTER_H
