#include "MarkdownHighlighter.h"
#include <QDebug>

MarkdownHighlighter::MarkdownHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
    , m_headingsEnabled(true)
    , m_boldItalicEnabled(true)
    , m_codeBlocksEnabled(true)
    , m_linksEnabled(true)
    , m_currentTheme("light")
{
    setupHighlighter();
    setupLightTheme();  // 默认使用亮色主题
}

void MarkdownHighlighter::setupHighlighter()
{
    highlightingRules.clear();

    // 注意：规则按优先级顺序添加，优先级高的先处理

    // 水平分割线（优先级最高）
    HighlightingRule rule;
    rule.priority = 1;
    rule.pattern = QRegularExpression("^ {0,3}(?:\\*\\s*){3,}$|^ {0,3}(?:-\\s*){3,}$|^ {0,3}(?:_\\s*){3,}$");
    rule.format = horizontalRuleFormat;
    highlightingRules.append(rule);

    // 标题（优先级中等）
    if (m_headingsEnabled) {
        for (int i = 1; i <= 6; ++i) {
            rule.priority = 10 + i;
            rule.pattern = QRegularExpression(QString("^#{%1}\\s+.+$").arg(i));
            rule.format = headingFormat;
            highlightingRules.append(rule);
        }

        rule.priority = 20;
        rule.pattern = QRegularExpression("^.+\\n[=]+$");
        rule.format = headingFormat;
        highlightingRules.append(rule);

        rule.priority = 21;
        rule.pattern = QRegularExpression("^.+\\n[-]+$");
        rule.format = headingFormat;
        highlightingRules.append(rule);
    }

    // 引用块
    rule.priority = 30;
    rule.pattern = QRegularExpression("^>.*$");
    rule.format = quoteFormat;
    highlightingRules.append(rule);

    // 列表
    rule.priority = 40;
    rule.pattern = QRegularExpression("^\\s*[-*+]\\s+");
    rule.format = listFormat;
    highlightingRules.append(rule);

    rule.priority = 41;
    rule.pattern = QRegularExpression("^\\s*\\d+\\.\\s+");
    rule.format = listFormat;
    highlightingRules.append(rule);

    if (m_boldItalicEnabled) {
        rule.priority = 50;
        rule.pattern = QRegularExpression("\\*\\*[^*]+\\*\\*|__[^_]+__");
        rule.format = boldFormat;
        highlightingRules.append(rule);

        rule.priority = 51;
        rule.pattern = QRegularExpression("\\*[^*]+\\*|_[^_]+_");
        rule.format = italicFormat;
        highlightingRules.append(rule);
    }

    rule.priority = 60;
    rule.pattern = QRegularExpression("`[^`]+`");
    rule.format = inlineCodeFormat;
    highlightingRules.append(rule);

    if (m_linksEnabled) {
        rule.priority = 70;
        rule.pattern = QRegularExpression("\\[[^\\]]+\\]\\([^\\)]+\\)");
        rule.format = linkFormat;
        highlightingRules.append(rule);

        rule.priority = 71;
        rule.pattern = QRegularExpression("!\\[[^\\]]+\\]\\([^\\)]+\\)");
        rule.format = linkFormat;
        highlightingRules.append(rule);
    }

    // 规则构建完成后排序一次（修复Bug 9：每行sort的O(n²)问题）
    std::sort(highlightingRules.begin(), highlightingRules.end(),
              [](const HighlightingRule &a, const HighlightingRule &b) {
                  return a.priority < b.priority;
              });
}

void MarkdownHighlighter::setupLightTheme()
{
    m_currentTheme = "light";

    // 标题格式 - 深蓝色，醒目但不刺眼
    headingFormat.setForeground(QColor(0x15, 0x65, 0xC0));  // 深蓝色
    headingFormat.setFontWeight(QFont::Bold);

    // 粗体格式 - 深绿色
    boldFormat.setFontWeight(QFont::Bold);
    boldFormat.setForeground(QColor(0x2E, 0x7D, 0x32));

    // 斜体格式 - 紫色
    italicFormat.setFontItalic(true);
    italicFormat.setForeground(QColor(0x7B, 0x1F, 0xA2));

    // 行内代码格式 - 深红色背景，浅灰色背景
    inlineCodeFormat.setForeground(QColor(0xC6, 0x28, 0x28));
    inlineCodeFormat.setBackground(QColor(0xF5, 0xF5, 0xF5));
    QStringList inlineCodeFonts;
    inlineCodeFonts << "Consolas" << "Monaco" << "Courier New" << "monospace";
    inlineCodeFormat.setFontFamilies(inlineCodeFonts);

    // 代码块格式 - 深色文字，浅灰色背景，非常明显
    blockCodeFormat.setForeground(QColor(0x37, 0x47, 0x4F));
    blockCodeFormat.setBackground(QColor(0xF8, 0xF9, 0xFA));  // 浅灰色背景
    QStringList blockCodeFonts;
    blockCodeFonts << "Consolas" << "Monaco" << "Courier New" << "monospace";
    blockCodeFormat.setFontFamilies(blockCodeFonts);

    // 链接格式 - 蓝色带下划线
    linkFormat.setForeground(QColor(0x02, 0x77, 0xBD));
    linkFormat.setFontUnderline(true);

    // 列表格式 - 棕色
    listFormat.setForeground(QColor(0x5D, 0x40, 0x37));

    // 引用格式 - 灰色斜体
    quoteFormat.setForeground(QColor(0x54, 0x6E, 0x7A));
    quoteFormat.setFontItalic(true);

    // 水平线格式 - 浅灰色
    horizontalRuleFormat.setForeground(QColor(0xBD, 0xBD, 0xBD));
    horizontalRuleFormat.setBackground(QColor(0xEE, 0xEE, 0xEE));
}
void MarkdownHighlighter::setupDarkTheme()
{
    m_currentTheme = "dark";

    // 标题格式 - 橙色，在暗色背景下很醒目
    headingFormat.setForeground(QColor(0xCC, 0x78, 0x32));  // 橙色
    headingFormat.setFontWeight(QFont::Bold);

    // 粗体格式 - 浅绿色
    boldFormat.setFontWeight(QFont::Bold);
    boldFormat.setForeground(QColor(0x6A, 0xBB, 0x7A));

    // 斜体格式 - 浅紫色
    italicFormat.setFontItalic(true);
    italicFormat.setForeground(QColor(0xB4, 0x7F, 0xDD));

    // 行内代码格式 - 红色文字，深灰色背景
    inlineCodeFormat.setForeground(QColor(0xE7, 0x4C, 0x3C));
    inlineCodeFormat.setBackground(QColor(0x3C, 0x3F, 0x41));
    QStringList inlineCodeFonts;
    inlineCodeFonts << "Consolas" << "Monaco" << "Courier New" << "monospace";
    inlineCodeFormat.setFontFamilies(inlineCodeFonts);

    // 代码块格式 - 浅色文字，深蓝色背景，非常明显
    blockCodeFormat.setForeground(QColor(0xEC, 0xF0, 0xF1));
    blockCodeFormat.setBackground(QColor(0x2B, 0x3B, 0x4E));  // 深蓝色背景
    QStringList blockCodeFonts;
    blockCodeFonts << "Consolas" << "Monaco" << "Courier New" << "monospace";
    blockCodeFormat.setFontFamilies(blockCodeFonts);

    // 链接格式 - 浅蓝色带下划线
    linkFormat.setForeground(QColor(0x34, 0x98, 0xDB));
    linkFormat.setFontUnderline(true);

    // 列表格式 - 浅灰色
    listFormat.setForeground(QColor(0x95, 0xA5, 0xA6));

    // 引用格式 - 灰色斜体
    quoteFormat.setForeground(QColor(0x7F, 0x8C, 0x8D));
    quoteFormat.setFontItalic(true);

    // 水平线格式 - 深灰色
    horizontalRuleFormat.setForeground(QColor(0x7F, 0x8C, 0x8D));
    horizontalRuleFormat.setBackground(QColor(0x2C, 0x3E, 0x50));
}

void MarkdownHighlighter::setTheme(const QString &themeName)
{
    if (themeName == "dark") {
        setupDarkTheme();
    } else {
        setupLightTheme();
    }

    rehighlight();  // 重新应用高亮
}

void MarkdownHighlighter::enableHeadings(bool enable)
{
    m_headingsEnabled = enable;
    setupHighlighter();
    rehighlight();
}

void MarkdownHighlighter::enableBoldAndItalic(bool enable)
{
    m_boldItalicEnabled = enable;
    setupHighlighter();
    rehighlight();
}

void MarkdownHighlighter::enableCodeBlocks(bool enable)
{
    m_codeBlocksEnabled = enable;
    setupHighlighter();
    rehighlight();
}

void MarkdownHighlighter::enableLinks(bool enable)
{
    m_linksEnabled = enable;
    setupHighlighter();
    rehighlight();
}

void MarkdownHighlighter::highlightBlock(const QString &text)
{
    if (text.isEmpty()) {
        return;
    }

    // 应用高亮规则（规则已在 setupHighlighter() 中按优先级排序完毕）
    for (const HighlightingRule &rule : highlightingRules) {
        QRegularExpressionMatchIterator matchIterator = rule.pattern.globalMatch(text);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    // 多行代码块状态机（修复Bug 8：正确处理跨行代码块）
    QRegularExpression codeBlockDelimiter("^```");
    if (previousBlockState() == 1) {
        // 当前在代码块内部 → 整行染代码块色
        setFormat(0, text.length(), blockCodeFormat);
        QRegularExpressionMatch endMatch = codeBlockDelimiter.match(text);
        if (endMatch.hasMatch()) {
            // 遇到结束标记 ``` → 状态切回0
            setCurrentBlockState(0);
        } else {
            setCurrentBlockState(1);
        }
    } else {
        // 不在代码块中 → 检查本行是否以 ``` 开头
        QRegularExpressionMatch startMatch = codeBlockDelimiter.match(text);
        if (startMatch.hasMatch()) {
            setFormat(0, text.length(), blockCodeFormat);
            // 检查同一行是否也有闭合标记（如 ```code```）
            QString afterStart = text.mid(startMatch.capturedEnd());
            if (afterStart.contains(QRegularExpression("```"))) {
                setCurrentBlockState(0); // 单行代码块
            } else {
                setCurrentBlockState(1); // 多行代码块开始
            }
        }
    }
}
