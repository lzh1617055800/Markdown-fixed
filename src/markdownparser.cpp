#include "MarkdownParser.h"
#include <QStringBuilder>
#include <algorithm>

// 初始化静态正则表达式（保持你原有的定义）
const QRegularExpression MarkdownParser::HEADER_REGEX(
    "^(#{1,6})\\s+(.*)$",
    QRegularExpression::MultilineOption
    );

const QRegularExpression MarkdownParser::BOLD_REGEX(
    "\\*\\*(.*?)\\*\\*"
    );

const QRegularExpression MarkdownParser::ITALIC_REGEX(
    "\\*(.*?)\\*"
    );

const QRegularExpression MarkdownParser::INLINE_CODE_REGEX(
    "`(.*?)`"
    );

const QRegularExpression MarkdownParser::BLOCK_CODE_REGEX(
    "```(.*?)```",
    QRegularExpression::DotMatchesEverythingOption
    );

const QRegularExpression MarkdownParser::LINK_REGEX(
    "\\[(.*?)\\]\\((.*?)\\)"
    );

const QRegularExpression MarkdownParser::IMAGE_REGEX(
    "!\\[(.*?)\\]\\((.*?)\\)"
    );

const QRegularExpression MarkdownParser::UNORDERED_LIST_REGEX(
    "^\\s*[-*+]\\s+(.*)$",
    QRegularExpression::MultilineOption
    );

QString MarkdownParser::toHtml(const QString &markdown)
{
    if (markdown.isEmpty()) {
        return ""; // 空文本返回空，避免多余标签
    }

    QString html = markdown;

    // 调整解析顺序：块级元素→行内元素→换行（避免嵌套冲突）
    html = parseBlockCode(html);      // 代码块（最高优先级）
    html = parseTables(html);         // 表格（块级）
    html = parseHeaders(html);        // 标题（块级）
    html = parseLists(html);          // 列表（块级）
    html = parseBoldAndItalic(html);  // 粗体/斜体（行内）
    html = parseInlineCode(html);     // 行内代码（行内）
    html = parseLinks(html);          // 链接（行内）
    html = parseImages(html);         // 图片（行内）
    html = parseLineBreaks(html);     // 最后处理换行

    return html;
}

QString MarkdownParser::toPlainText(const QString &markdown)
{
    QString text = markdown;

    // 移除Markdown标记，保留纯文本
    text.remove(HEADER_REGEX);
    text.remove(BOLD_REGEX);
    text.remove(ITALIC_REGEX);
    text.remove(INLINE_CODE_REGEX);
    text.remove(LINK_REGEX);
    text.remove(IMAGE_REGEX);

    return text.trimmed();
}

QString MarkdownParser::parseHeaders(const QString &text)
{
    QString result = text;
    QStringList lines = result.split('\n');

    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i];

        // 匹配 # 标题
        QRegularExpressionMatch match = HEADER_REGEX.match(line);
        if (match.hasMatch()) {
            QString levelStr = match.captured(1);
            QString content = match.captured(2).trimmed();
            int headerLevel = levelStr.length();
            lines[i] = QString("<h%1>%2</h%1>").arg(headerLevel).arg(content);
            continue; // 处理完#标题，跳过底线式检查
        }

        // 处理底线式标题（===对应h1，---对应h2）
        if (i > 0 && i < lines.size() - 1) { // 避免越界
            QString prevLine = lines[i-1].trimmed();
            QString currLine = line.trimmed();
            if (!prevLine.isEmpty() && currLine.startsWith("===")) {
                lines[i-1] = QString("<h1>%1</h1>").arg(prevLine);
                lines[i] = ""; // 清空底线行
            } else if (!prevLine.isEmpty() && currLine.startsWith("---")) {
                lines[i-1] = QString("<h2>%1</h2>").arg(prevLine);
                lines[i] = ""; // 清空底线行
            }
        }
    }

    return lines.join('\n');
}

QString MarkdownParser::parseBoldAndItalic(const QString &text)
{
    QString result = text;

    // 先处理粗体
    result.replace(BOLD_REGEX, "<strong>\\1</strong>");

    // 再处理斜体（避免与粗体冲突）
    result.replace(ITALIC_REGEX, "<em>\\1</em>");

    return result;
}

QString MarkdownParser::parseInlineCode(const QString &text)
{
    QString result = text;
    result.replace(INLINE_CODE_REGEX, "<code>\\1</code>");
    return result;
}

QString MarkdownParser::parseBlockCode(const QString &text)
{
    QString result = text;

    // 简单的代码块处理
    int startPos = 0;
    while ((startPos = result.indexOf("```", startPos)) != -1) {
        int endPos = result.indexOf("```", startPos + 3);
        if (endPos == -1) break;

        QString codeContent = result.mid(startPos + 3, endPos - startPos - 3);
        codeContent = codeContent.trimmed();

        // HTML转义
        codeContent = codeContent.toHtmlEscaped();
        codeContent.replace("\n", "<br>");
        codeContent.replace("  ", "&nbsp;&nbsp;");

        // 修复Bug 20：使用语义化HTML，样式交给CSS控制
        QString replacement = QString("<pre><code>%1</code></pre>").arg(codeContent);

        result.replace(startPos, endPos - startPos + 3, replacement);
        startPos += replacement.length();  // 移动到替换后的位置
    }

    return result;
}

QString MarkdownParser::parseLinks(const QString &text)
{
    QString result = text;
    result.replace(LINK_REGEX, "<a href=\"\\2\">\\1</a>");
    return result;
}

QString MarkdownParser::parseImages(const QString &text)
{
    QString result = text;
    result.replace(IMAGE_REGEX, "<img src=\"\\2\" alt=\"\\1\">");
    return result;
}

QString MarkdownParser::parseLists(const QString &text)
{
    QString result = text;

    // ----------------- 处理无序列表（-/*/+） -----------------
    {
        QRegularExpression ulRegex(
            "(^|\\n)(\\s*[-*+]\\s+.*?(?:\n\\s*[-*+]\\s+.*?)*)",
            QRegularExpression::DotMatchesEverythingOption
            );

        QString processed = result;
        int offset = 0;
        QRegularExpressionMatchIterator ulIt = ulRegex.globalMatch(processed);

        while (ulIt.hasNext()) {
            QRegularExpressionMatch match = ulIt.next();
            int start = match.capturedStart() + offset;
            int length = match.capturedLength();
            QString listBlock = match.captured().trimmed();

            QStringList items = listBlock.split('\n', Qt::SkipEmptyParts);
            QStringList liItems;
            for (QString &item : items) {
                item.remove(QRegularExpression("^\\s*[-*+]\\s+"));
                liItems << QString("<li>%1</li>").arg(item.trimmed());
            }

            QString ulHtml = QString("<ul>%1</ul>").arg(liItems.join(""));
            processed.replace(start, length, ulHtml);
            offset += ulHtml.length() - length;
        }
        result = processed;
    }

    // ----------------- 处理有序列表（数字.）(Bug 14修复) -----------------
    {
        QRegularExpression olRegex(
            "(^|\\n)(\\s*\\d+\\.\\s+.*?(?:\n\\s*\\d+\\.\\s+.*?)*)",
            QRegularExpression::DotMatchesEverythingOption
            );

        QString processed = result;
        int offset = 0;
        QRegularExpressionMatchIterator olIt = olRegex.globalMatch(processed);

        while (olIt.hasNext()) {
            QRegularExpressionMatch match = olIt.next();
            int start = match.capturedStart() + offset;
            int length = match.capturedLength();
            QString listBlock = match.captured().trimmed();

            QStringList items = listBlock.split('\n', Qt::SkipEmptyParts);
            QStringList liItems;
            for (QString &item : items) {
                item.remove(QRegularExpression("^\\s*\\d+\\.\\s+"));
                liItems << QString("<li>%1</li>").arg(item.trimmed());
            }

            QString olHtml = QString("<ol>%1</ol>").arg(liItems.join(""));
            processed.replace(start, length, olHtml);
            offset += olHtml.length() - length;
        }
        result = processed;
    }

    return result;
}

QString MarkdownParser::parseTables(const QString &text)
{
    QString result = text;
    QStringList lines = result.split('\n');
    QStringList tableLines;
    bool inTable = false;

    for (int i = 0; i < lines.size(); ++i) {
        QString line = lines[i].trimmed();
        bool isTableLine = !line.isEmpty() && (line.startsWith('|') || line.endsWith('|')) && line.contains('|');

        if (isTableLine) {
            if (inTable && i > 0) {
                QString prevLine = lines[i-1].trimmed();
                bool isSeparator = line.contains(QRegularExpression("^\\|?\\s*(-+\\s*\\|\\s*)*-+\\s*\\|?$"));
                if (isSeparator && !prevLine.isEmpty() && prevLine.contains('|')) {
                    inTable = true;
                    tableLines.append(line);
                    continue;
                }
            }
            inTable = true;
            tableLines.append(line);
        } else {
            if (inTable && tableLines.size() >= 2) {
                QString tableHtml = "<table style=\"border-collapse: collapse; width: 100%; margin: 10px 0;\">";
                QString headerLine = tableLines[0].trimmed();
                headerLine.remove(QRegularExpression("^\\|"));
                headerLine.remove(QRegularExpression("\\|$"));
                QStringList headers = headerLine.split(QRegularExpression("\\|"), Qt::SkipEmptyParts);
                tableHtml += "<thead style=\"background-color: #f5f5f5;\"><tr>";
                for (const QString &header : headers) {
                    tableHtml += QString("<th style=\"border: 1px solid #ddd; padding: 8px; text-align: left;\">%1</th>")
                    .arg(header.trimmed());
                }
                tableHtml += "</tr></thead>";
                tableHtml += "<tbody>";
                for (int j = 2; j < tableLines.size(); ++j) {
                    QString bodyLine = tableLines[j].trimmed();
                    bodyLine.remove(QRegularExpression("^\\|"));
                    bodyLine.remove(QRegularExpression("\\|$"));
                    QStringList cells = bodyLine.split(QRegularExpression("\\|"), Qt::SkipEmptyParts);
                    tableHtml += "<tr>";
                    for (const QString &cell : cells) {
                        tableHtml += QString("<td style=\"border: 1px solid #ddd; padding: 8px; text-align: left;\">%1</td>")
                        .arg(cell.trimmed());
                    }
                    tableHtml += "</tr>";
                }
                tableHtml += "</tbody></table>";

                int startLine = i - tableLines.size();
                // 新增：定义endLine（表格结束的行索引，即当前行的前一行）
                int endLine = i - 1;
                int startPos = 0;
                for (int k = 0; k < startLine; ++k) {
                    startPos += lines[k].length() + 1;
                }
                int endPos = startPos;
                // 现在endLine已定义，可正常使用
                for (int k = startLine; k <= endLine; ++k) {
                    endPos += lines[k].length() + 1;
                }
                result.replace(startPos, endPos - startPos, tableHtml);

                tableLines.clear();
                inTable = false;
            }
        }
    }

    // 处理最后一行可能残留的表格（这部分代码中未使用endLine，无需修改）
    if (inTable && tableLines.size() >= 2) {
        QString tableHtml = "<table style=\"border-collapse: collapse; width: 100%; margin: 10px 0;\">";
        QString headerLine = tableLines[0].trimmed();
        headerLine.remove(QRegularExpression("^\\|"));
        headerLine.remove(QRegularExpression("\\|$"));
        QStringList headers = headerLine.split(QRegularExpression("\\|"), Qt::SkipEmptyParts);
        tableHtml += "<thead style=\"background-color: #f5f5f5;\"><tr>";
        for (const QString &header : headers) {
            tableHtml += QString("<th style=\"border: 1px solid #ddd; padding: 8px; text-align: left;\">%1</th>")
            .arg(header.trimmed());
        }
        tableHtml += "</tr></thead>";
        tableHtml += "<tbody>";
        for (int j = 2; j < tableLines.size(); ++j) {
            QString bodyLine = tableLines[j].trimmed();
            bodyLine.remove(QRegularExpression("^\\|"));
            bodyLine.remove(QRegularExpression("\\|$"));
            QStringList cells = bodyLine.split(QRegularExpression("\\|"), Qt::SkipEmptyParts);
            tableHtml += "<tr>";
            for (const QString &cell : cells) {
                tableHtml += QString("<td style=\"border: 1px solid #ddd; padding: 8px; text-align: left;\">%1</td>")
                .arg(cell.trimmed());
            }
            tableHtml += "</tr>";
        }
        tableHtml += "</tbody></table>";

        int startLine = lines.size() - tableLines.size();
        int startPos = 0;
        for (int k = 0; k < startLine; ++k) {
            startPos += lines[k].length() + 1;
        }
        int endPos = result.length();
        result.replace(startPos, endPos - startPos, tableHtml);
    }

    return result;
}
QString MarkdownParser::parseLineBreaks(const QString &text)
{
    QString result = text;

    // 步骤1：标记块级元素，避免被<p>包裹
    QString temp = result;
    QRegularExpression blockRegex("<(h[1-6]|pre|ul|ol|li|table|thead|tbody|tr|th|td|blockquote)>.*?</\\1>",
                                  QRegularExpression::DotMatchesEverythingOption | QRegularExpression::CaseInsensitiveOption);
    QList<QString> blockContents;
    int blockIndex = 0;
    QRegularExpressionMatchIterator matchIt = blockRegex.globalMatch(temp);
    QString replacedTemp = temp; // 用于存储替换后的临时字符串
    int offset = 0; // 记录替换导致的文本长度偏移，确保位置计算正确

    while (matchIt.hasNext()) {
        QRegularExpressionMatch match = matchIt.next();
        if (!match.hasMatch()) continue;

        // 获取匹配内容在原始字符串中的位置（需加上偏移量）
        int start = match.capturedStart() + offset;
        int length = match.capturedLength();
        QString capturedText = match.captured(); // 获取匹配到的块级元素内容

        // 存储原始块级内容，生成占位符
        blockContents << capturedText;
        QString placeholder = QString("__BLOCK_%1__").arg(blockIndex++);

        // 替换当前匹配位置为占位符
        replacedTemp.replace(start, length, placeholder);

        // 更新偏移量（占位符长度 - 原匹配内容长度）
        offset += placeholder.length() - length;
    }

    // 将处理后的字符串赋值给temp
    temp = replacedTemp;

    // 步骤2：处理普通文本的段落和换行
    temp.replace("\n\n", "</p><p>"); // 段落分隔
    temp.replace("\n", "<br>");      // 单行换行
    if (!temp.isEmpty() && !temp.contains(QRegularExpression("^<[a-z]+>"))) {
        temp = "<p>" + temp + "</p>";
    }

    // 步骤3：恢复块级元素
    for (int i = 0; i < blockIndex; ++i) {
        temp.replace(QString("__BLOCK_%1__").arg(i), blockContents[i]);
    }

    // 步骤4：清理多余标签
    temp.remove(QRegularExpression("<p>\\s*</p>"));

    return temp;
}
