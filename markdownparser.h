#ifndef MARKDOWNPARSER_H
#define MARKDOWNPARSER_H

#include <QString>
#include <QRegularExpression>

class MarkdownParser
{
public:
    static QString toHtml(const QString &markdown);
    static QString toPlainText(const QString &markdown);

private:
    // 静态正则表达式（保持你原有的定义）
    static const QRegularExpression HEADER_REGEX;
    static const QRegularExpression BOLD_REGEX;
    static const QRegularExpression ITALIC_REGEX;
    static const QRegularExpression INLINE_CODE_REGEX;
    static const QRegularExpression BLOCK_CODE_REGEX;
    static const QRegularExpression LINK_REGEX;
    static const QRegularExpression IMAGE_REGEX;
    static const QRegularExpression UNORDERED_LIST_REGEX;

    // 解析函数
    static QString parseHeaders(const QString &text);
    static QString parseBoldAndItalic(const QString &text);
    static QString parseInlineCode(const QString &text);
    static QString parseBlockCode(const QString &text);
    static QString parseLinks(const QString &text);
    static QString parseImages(const QString &text);
    static QString parseLists(const QString &text);
    static QString parseTables(const QString &text);
    static QString parseLineBreaks(const QString &text);
};

#endif // MARKDOWNPARSER_H
