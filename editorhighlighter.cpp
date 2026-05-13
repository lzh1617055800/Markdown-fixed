#include "EditorHighlighter.h"
#include <QFont>

// 构造函数：初始化主题（默认亮色）
EditorHighlighter::EditorHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    setupLightTheme();
}
void EditorHighlighter::setTheme(const QString &theme)
{
    if (theme == "dark") {
        setupDarkTheme();
    } else {
        setupLightTheme();
    }
    // 主题切换后重新高亮所有文本
    rehighlight();
}

// 初始化亮色主题样式
void EditorHighlighter::setupLightTheme()
{
    // 标题：深蓝色、加粗
    m_headingFormat.setForeground(QColor(0x15, 0x65, 0xC0)); // #1565C0
    m_headingFormat.setFontWeight(QFont::Bold);

    // 粗体：深绿色、加粗
    m_boldFormat.setForeground(QColor(0x2E, 0x7D, 0x32)); // #2E7D32
    m_boldFormat.setFontWeight(QFont::Bold);

    // 斜体：紫色、斜体
    m_italicFormat.setForeground(QColor(0x7B, 0x1F, 0xA2)); // #7B1FA2
    m_italicFormat.setFontItalic(true);

    // 行内代码：深红色、灰色背景、等宽字体
    m_inlineCodeFormat.setForeground(QColor(0xC6, 0x28, 0x28)); // #C62828
    m_inlineCodeFormat.setBackground(QColor(0xF5, 0xF5, 0xF5)); // #F5F5F5
    m_inlineCodeFormat.setFontFamily("Consolas, Monaco, monospace");

    // 代码块：深灰色、浅灰背景、等宽字体
    m_blockCodeFormat.setForeground(QColor(0x37, 0x47, 0x4F)); // #37474F
    m_blockCodeFormat.setBackground(QColor(0xFA, 0xFA, 0xFA)); // #FAFAFA
    m_blockCodeFormat.setFontFamily("Consolas, Monaco, monospace");

    // 链接：蓝色、下划线
    m_linkFormat.setForeground(QColor(0x02, 0x77, 0xBD)); // #0277BD
    m_linkFormat.setFontUnderline(true);

    // 列表：深棕色
    m_listFormat.setForeground(QColor(0x5D, 0x40, 0x37)); // #5D4037

    // 引用：深灰色、斜体
    m_quoteFormat.setForeground(QColor(0x54, 0x6E, 0x7A)); // #546E7A
    m_quoteFormat.setFontItalic(true);
}

// 初始化暗色主题样式
void EditorHighlighter::setupDarkTheme()
{
    // 标题：浅蓝色
    m_headingFormat.setForeground(QColor(0x81, 0xD4, 0xFA)); // #81D4FA
    m_headingFormat.setFontWeight(QFont::Bold);

    // 粗体：浅绿色
    m_boldFormat.setForeground(QColor(0x81, 0xC7, 0x84)); // #81C784
    m_boldFormat.setFontWeight(QFont::Bold);

    // 斜体：浅紫色
    m_italicFormat.setForeground(QColor(0xBA, 0x68, 0xC8)); // #BA68C8
    m_italicFormat.setFontItalic(true);

    // 行内代码：浅红色、深灰背景
    m_inlineCodeFormat.setForeground(QColor(0xEF, 0x9A, 0x9A)); // #EF9A9A
    m_inlineCodeFormat.setBackground(QColor(0x34, 0x49, 0x5E)); // #34495E
    m_inlineCodeFormat.setFontFamily("Consolas, Monaco, monospace");

    // 代码块：浅灰色、深色背景
    m_blockCodeFormat.setForeground(QColor(0xEC, 0xF0, 0xF1)); // #ECF0F1
    m_blockCodeFormat.setBackground(QColor(0x2C, 0x3E, 0x50)); // #2C3E50
    m_blockCodeFormat.setFontFamily("Consolas, Monaco, monospace");

    // 链接：浅蓝色、下划线
    m_linkFormat.setForeground(QColor(0x42, 0xA5, 0xF5)); // #42A5F5
    m_linkFormat.setFontUnderline(true);

    // 列表：浅棕色
    m_listFormat.setForeground(QColor(0xBC, 0xAAA, 0x99)); // #BCAAA9
}

// 核心函数：对每一行文本进行语法解析和染色
void EditorHighlighter::highlightBlock(const QString &text)
{
    // 1. 处理代码块（Bug 4修复：m_inCodeBlock 成员变量替代 static）
    QRegularExpression codeBlockStart("^```");
    QRegularExpression codeBlockEnd("```$");

    if (codeBlockStart.match(text).hasMatch()) {
        m_inCodeBlock = true;
        setFormat(0, text.length(), m_blockCodeFormat);
        return;
    }
    if (m_inCodeBlock) {
        setFormat(0, text.length(), m_blockCodeFormat);
        if (codeBlockEnd.match(text).hasMatch()) {
            m_inCodeBlock = false;
        }
        return;
    }

    // 2. 处理标题（# 开头，1-6级）
    QRegularExpression headingRegex("^(#{1,6})\\s+(.+)$");
    QRegularExpressionMatch headingMatch = headingRegex.match(text);
    if (headingMatch.hasMatch()) {
        int headingLength = headingMatch.captured(1).length(); // #的数量
        int textLength = text.length();
        setFormat(0, textLength, m_headingFormat);
        return;
    }

    // 3. 处理引用（> 开头）
    QRegularExpression quoteRegex("^>.*$");
    if (quoteRegex.match(text).hasMatch()) {
        setFormat(0, text.length(), m_quoteFormat);
        return;
    }

    // 4. 处理列表（-/*/+ 开头，或数字. 开头）
    static QRegularExpression unorderedListRegex("^\\s*[-*+]\\s+");//无序标题正则匹配
    static QRegularExpression orderedListRegex("^\\s*\\d+\\.\\s+");//有序标题正则匹配
    QRegularExpressionMatch listMatch = unorderedListRegex.match(text);
    if (!listMatch.hasMatch()) {
        listMatch = orderedListRegex.match(text);
    }
    if (listMatch.hasMatch()) {
        int listMarkerLength = listMatch.capturedLength(); // 列表标记长度（如 "- " 或 "1. "）
        setFormat(0, listMarkerLength, m_listFormat); // 列表标记染色
        // 列表内容不染色（继承默认文本色）
        return;
    }

    // 5. 处理粗体（**内容** 或 __内容__）
    static QRegularExpression boldRegex("\\*\\*[^*]+\\*\\*|__[^_]+__");
    QRegularExpressionMatchIterator boldIter = boldRegex.globalMatch(text);
    while (boldIter.hasNext()) {
        QRegularExpressionMatch match = boldIter.next();
        setFormat(match.capturedStart(), match.capturedLength(), m_boldFormat);
    }

    // 6. 处理斜体（*内容* 或 _内容_）
    static QRegularExpression italicRegex("\\*[^*]+\\*|_[^_]+_");
    QRegularExpressionMatchIterator italicIter = italicRegex.globalMatch(text);
    while (italicIter.hasNext()) {
        QRegularExpressionMatch match = italicIter.next();
        setFormat(match.capturedStart(), match.capturedLength(), m_italicFormat);
    }

    // 7. 处理行内代码（`内容`）
    static QRegularExpression inlineCodeRegex("`[^`]+`");
    QRegularExpressionMatchIterator codeIter = inlineCodeRegex.globalMatch(text);
    while (codeIter.hasNext()) {
        QRegularExpressionMatch match = codeIter.next();
        setFormat(match.capturedStart(), match.capturedLength(), m_inlineCodeFormat);
    }

    // 8. 处理链接（[文字](链接)）
    static QRegularExpression linkRegex("\\[[^\\]]+\\]\\([^\\)]+\\)");
    QRegularExpressionMatchIterator linkIter = linkRegex.globalMatch(text);
    while (linkIter.hasNext()) {
        QRegularExpressionMatch match = linkIter.next();
        setFormat(match.capturedStart(), match.capturedLength(), m_linkFormat);
    }
}
