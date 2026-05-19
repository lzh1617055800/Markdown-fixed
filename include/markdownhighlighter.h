#ifndef MARKDOWNHIGHLIGHTER_H
#define MARKDOWNHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>
#include <QVector>

class MarkdownHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit MarkdownHighlighter(QTextDocument *parent = nullptr);

    // 设置高亮主题
    void setTheme(const QString &themeName);

    // 启用/禁用特定语法的高亮
    void enableHeadings(bool enable);
    void enableBoldAndItalic(bool enable);
    void enableCodeBlocks(bool enable);
    void enableLinks(bool enable);

protected:
    void highlightBlock(const QString &text) override;

private:
    // 初始化高亮规则
    void setupHighlighter();
    void setupLightTheme();
    void setupDarkTheme();

    // 高亮规则结构
    struct HighlightingRule
    {
        QRegularExpression pattern;
        QTextCharFormat format;
        int priority;  // 优先级，数字越小优先级越高
    };

    // 高亮规则列表
    QVector<HighlightingRule> highlightingRules;

    // 特定语法的高亮格式
    QTextCharFormat headingFormat;
    QTextCharFormat boldFormat;
    QTextCharFormat italicFormat;
    QTextCharFormat inlineCodeFormat;
    QTextCharFormat blockCodeFormat;
    QTextCharFormat linkFormat;
    QTextCharFormat listFormat;
    QTextCharFormat quoteFormat;
    QTextCharFormat horizontalRuleFormat;

    // 启用标志
    bool m_headingsEnabled;
    bool m_boldItalicEnabled;
    bool m_codeBlocksEnabled;
    bool m_linksEnabled;

    // 当前主题
    QString m_currentTheme;
};

#endif // MARKDOWNHIGHLIGHTER_H
