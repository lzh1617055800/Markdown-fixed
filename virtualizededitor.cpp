#include "VirtualizedEditor.h"
#include <QFontMetrics>
#include <QTextCursor>
#include <QDebug>

VirtualizedEditor::VirtualizedEditor(QWidget *parent)
    : QTextEdit(parent),
    m_visibleStartLine(0), m_visibleEndLine(0), m_linesPerView(0),
    m_globalCursorLine(0), m_globalCursorColumn(0)
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    connect(verticalScrollBar(), &QScrollBar::valueChanged,
            this, &VirtualizedEditor::updateVisibleLines);
    connect(this, &QTextEdit::textChanged, this,
            &VirtualizedEditor::onTextChanged);
    connect(this, &QTextEdit::cursorPositionChanged, this, [this]() {
        QTextCursor cursor = textCursor();
        m_globalCursorLine = m_visibleStartLine + cursor.blockNumber();
        m_globalCursorColumn = cursor.columnNumber();
    });
}

void VirtualizedEditor::setFullContent(const QString &content)
{
    m_fullContent = content;
    m_fullLines = content.split('\n');

    if (m_linesPerView <= 0) {
        QFontMetrics fm(font());
        m_linesPerView = height() / fm.lineSpacing();
        if (m_linesPerView <= 0) m_linesPerView = 20;
    }

    updateVisibleLines();
    m_lastVisibleStart = -1; // 强制 paintEvent 首次重建完整文档
    verticalScrollBar()->setValue(0);
}

//每次变更内容QT自动调用重绘函数，做出重载处理
void VirtualizedEditor::paintEvent(QPaintEvent *event)
{
    QFontMetrics fm(font());
    m_linesPerView = height() / fm.lineSpacing();
    if (m_linesPerView <= 0) m_linesPerView = 20;

    updateVisibleLines();

    if (m_visibleStartLine != m_lastVisibleStart) {
        m_lastVisibleStart = m_visibleStartLine;
        int tempGlobalLine = m_globalCursorLine;
        int tempGlobalCol = m_globalCursorColumn;

        int savedScroll = verticalScrollBar()->value();

        m_isProgrammaticChange = true;
        rebuildFullDocument();
        m_isProgrammaticChange = false;

        // rebuildFullDocument 内部 setPlainText 已含全文行数→滚动条自动正确
        verticalScrollBar()->setValue(savedScroll);

        if (tempGlobalLine >= m_visibleStartLine && tempGlobalLine <= m_visibleEndLine) {
            QTextCursor cursor = textCursor();
            int localLine = tempGlobalLine - m_visibleStartLine;
            cursor.movePosition(QTextCursor::Start);
            for (int i = 0; i < localLine; ++i)
                cursor.movePosition(QTextCursor::Down);
            cursor.movePosition(QTextCursor::Right, QTextCursor::MoveAnchor, tempGlobalCol);
            setTextCursor(cursor);
        }
    }

    QTextEdit::paintEvent(event);
}

void VirtualizedEditor::scrollContentsBy(int dx, int dy)
{
    QTextEdit::scrollContentsBy(dx, dy);
    updateVisibleLines();
}

void VirtualizedEditor::updateVisibleLines()
{
    if (m_fullLines.isEmpty()) {
        m_visibleStartLine = 0;
        m_visibleEndLine = 0;
        return;
    }

    QFontMetrics fm(font());
    int lineHeight = fm.lineSpacing();
    if (lineHeight <= 0) lineHeight = 16;

    int scrollValue = verticalScrollBar()->value();
    m_visibleStartLine = scrollValue / lineHeight;
    m_visibleEndLine = m_visibleStartLine + m_linesPerView - 1;

    if (m_visibleEndLine >= m_fullLines.size())
        m_visibleEndLine = m_fullLines.size() - 1;
    if (m_visibleStartLine >= m_fullLines.size())
        m_visibleStartLine = qMax(0, m_fullLines.size() - m_linesPerView);
}

// 构造完整文档：文档第 i 行 = m_fullLines[i]。
// 可见区（含 buffer）填真实内容，其余填空行。总行数 = m_fullLines.size()。
void VirtualizedEditor::rebuildFullDocument()
{
    const int total = m_fullLines.size();
    const int buf = 2;
    int contentStart = qMax(0, m_visibleStartLine - buf);
    int contentEnd = qMin(total - 1, m_visibleEndLine + buf);

    QStringList docLines;
    docLines.reserve(total);

    // 可见区之前的空行
    for (int i = 0; i < contentStart; ++i)
        docLines << QString();

    // 可见区真实内容
    for (int i = contentStart; i <= contentEnd; ++i)
        docLines << m_fullLines[i];

    // 可见区之后的空行
    for (int i = contentEnd + 1; i < total; ++i)
        docLines << QString();

    setPlainText(docLines.join('\n'));
    // 此时 QTextEdit 看到 total 行 → 滚动条自动正确，不需手动 sync
}

void VirtualizedEditor::onTextChanged()
{
    if (m_isProgrammaticChange) return;

    // 从完整文档提取可见区对应的行（精确对齐：文档第 N 行 = m_fullLines[N]）
    QStringList allDocLines = toPlainText().split('\n');
    if (m_fullLines.isEmpty()) {
        m_fullContent = toPlainText();
        m_fullLines  = allDocLines;
        return;
    }
    const int buf = 2;
    int contentStart = qMax(0, m_visibleStartLine - buf);
    int contentEnd = qMin(m_fullLines.size() - 1, m_visibleEndLine + buf);

    // 检查可见区是否真的变了
    QStringList newVisible;
    for (int i = contentStart; i <= contentEnd && i < allDocLines.size(); ++i)
        newVisible << allDocLines[i];
    QStringList oldVisible;
    for (int i = contentStart; i <= contentEnd && i < m_fullLines.size(); ++i)
        oldVisible << m_fullLines[i];
    if (newVisible.join('\n') == oldVisible.join('\n')) return;

    // 逐行合并回全文（文档和全文行号严格对应）
    QStringList fullLines = m_fullContent.split('\n');
    for (int i = contentStart; i <= contentEnd && i < allDocLines.size(); ++i) {
        if (i < fullLines.size())
            fullLines[i] = allDocLines[i];
        else
            fullLines.append(allDocLines[i]);
    }
    m_fullContent = fullLines.join('\n');
    m_fullLines = fullLines;
}
