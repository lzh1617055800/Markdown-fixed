#include "VirtualizedEditor.h"

#include <QFontMetrics>
#include <QTextCursor>

VirtualizedEditor::VirtualizedEditor(QWidget *parent)
    : QTextEdit(parent),
      m_visibleStartLine(0),
      m_visibleEndLine(0),
      m_linesPerView(0),
      m_globalCursorLine(0),
      m_globalCursorColumn(0)
{
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);

    connect(verticalScrollBar(), &QScrollBar::valueChanged,
            this, &VirtualizedEditor::updateVisibleLines);
    connect(this, &QTextEdit::textChanged,
            this, &VirtualizedEditor::onTextChanged);
    connect(this, &QTextEdit::cursorPositionChanged, this, [this]() {
        const QTextCursor cursor = textCursor();
        m_globalCursorLine = m_visibleStartLine + cursor.blockNumber();
        m_globalCursorColumn = cursor.columnNumber();
    });
}

void VirtualizedEditor::setFullContent(const QString &content)
{
    m_fullContent = content;
    m_fullLines = content.split('\n');

    if (m_linesPerView <= 0) {
        const QFontMetrics fm(font());
        m_linesPerView = height() / fm.lineSpacing();
        if (m_linesPerView <= 0) {
            m_linesPerView = 20;
        }
    }

    updateVisibleLines();
    m_lastVisibleStart = -1;
    verticalScrollBar()->setValue(0);
}

void VirtualizedEditor::paintEvent(QPaintEvent *event)
{
    const QFontMetrics fm(font());
    m_linesPerView = height() / fm.lineSpacing();
    if (m_linesPerView <= 0) {
        m_linesPerView = 20;
    }

    updateVisibleLines();

    if (m_visibleStartLine != m_lastVisibleStart) {
        m_lastVisibleStart = m_visibleStartLine;
        const int tempGlobalLine = m_globalCursorLine;
        const int tempGlobalCol = m_globalCursorColumn;
        const int savedScroll = verticalScrollBar()->value();

        m_isProgrammaticChange = true;
        rebuildFullDocument();
        m_isProgrammaticChange = false;

        verticalScrollBar()->setValue(savedScroll);

        if (tempGlobalLine >= m_visibleStartLine && tempGlobalLine <= m_visibleEndLine) {
            QTextCursor cursor = textCursor();
            const int localLine = tempGlobalLine - m_visibleStartLine;
            cursor.movePosition(QTextCursor::Start);
            for (int i = 0; i < localLine; ++i) {
                cursor.movePosition(QTextCursor::Down);
            }
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

    const QFontMetrics fm(font());
    int lineHeight = fm.lineSpacing();
    if (lineHeight <= 0) {
        lineHeight = 16;
    }

    const int scrollValue = verticalScrollBar()->value();
    m_visibleStartLine = scrollValue / lineHeight;
    m_visibleEndLine = m_visibleStartLine + m_linesPerView - 1;

    if (m_visibleEndLine >= m_fullLines.size()) {
        m_visibleEndLine = m_fullLines.size() - 1;
    }
    if (m_visibleStartLine >= m_fullLines.size()) {
        m_visibleStartLine = qMax(0, m_fullLines.size() - m_linesPerView);
    }
}

void VirtualizedEditor::rebuildFullDocument()
{
    const int total = m_fullLines.size();
    const int bufferLines = 2;
    const int contentStart = qMax(0, m_visibleStartLine - bufferLines);
    const int contentEnd = qMin(total - 1, m_visibleEndLine + bufferLines);

    QStringList docLines;
    docLines.reserve(total);

    for (int i = 0; i < contentStart; ++i) {
        docLines << QString();
    }

    for (int i = contentStart; i <= contentEnd; ++i) {
        docLines << m_fullLines[i];
    }

    for (int i = contentEnd + 1; i < total; ++i) {
        docLines << QString();
    }

    setPlainText(docLines.join('\n'));
}

void VirtualizedEditor::onTextChanged()
{
    if (m_isProgrammaticChange) {
        return;
    }

    // Use the current on-screen document as the source of truth for this edit.
    // If we limit the sync range by the old full-content model, first input in an
    // empty file and newly added lines can be silently lost.
    const QString currentDocumentText = toPlainText();
    const QStringList allDocLines = currentDocumentText.split('\n');

    const int bufferLines = 2;
    const int contentStart = qMax(0, m_visibleStartLine - bufferLines);
    const int contentEnd = qMin(m_visibleEndLine + bufferLines, allDocLines.size() - 1);

    if (contentEnd < contentStart) {
        m_fullContent.clear();
        m_fullLines.clear();
        return;
    }

    QStringList newVisible;
    for (int i = contentStart; i <= contentEnd && i < allDocLines.size(); ++i) {
        newVisible << allDocLines[i];
    }

    QStringList oldVisible;
    for (int i = contentStart; i <= contentEnd; ++i) {
        if (i >= 0 && i < m_fullLines.size()) {
            oldVisible << m_fullLines[i];
        } else {
            oldVisible << QString();
        }
    }

    if (newVisible == oldVisible) {
        return;
    }

    // The full model may not have these lines yet, so grow it first.
    QStringList fullLines = m_fullLines;
    while (fullLines.size() <= contentEnd) {
        fullLines.append(QString());
    }

    for (int i = contentStart; i <= contentEnd && i < allDocLines.size(); ++i) {
        fullLines[i] = allDocLines[i];
    }

    // 行数对齐：文档变大补空行/文档变小删多余
    while (fullLines.size() < allDocLines.size()) {
        fullLines.append(QString());
    }
    while (fullLines.size() > allDocLines.size()) {
        fullLines.removeLast();
    }

    m_fullLines = fullLines;
    m_fullContent = m_fullLines.join('\n');
}
