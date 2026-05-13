#include "DiffRenderer.h"
#include <QTextCursor>

DiffRenderer::DiffRenderer(QObject *parent)
    : QObject(parent), m_document(nullptr)
{
}

void DiffRenderer::setDocument(QTextDocument *doc)
{
    if (m_document) {
        disconnect(m_document, &QTextDocument::contentsChange,
                   this, &DiffRenderer::onDocumentContentsChanged);
    }

    m_document = doc;
    if (m_document) {
        // 连接文档内容变化信号
        connect(m_document, &QTextDocument::contentsChange,
                this, &DiffRenderer::onDocumentContentsChanged);
    }
}

QSet<int> DiffRenderer::getChangedLines()
{
    return m_changedLines;
}

void DiffRenderer::resetChangedLines()
{
    m_changedLines.clear();
}

// 核心：计算内容变化影响的行号
void DiffRenderer::onDocumentContentsChanged(int position, int charsRemoved, int charsAdded)
{
    if (!m_document) return;

    // 计算变化区域的起始行和结束行
    int startLine = positionToLineNumber(position);
    // 计算变化后的结束位置（考虑添加/删除的字符）
    int endPosition = qMax(0, position - charsRemoved + charsAdded);
    int endLine = positionToLineNumber(endPosition);

    // 将范围内的所有行标记为变化
    for (int line = startLine; line <= endLine; ++line) {
        m_changedLines.insert(line);
    }

    // 特殊处理：如果删除/添加导致行数变化，相邻行也可能受影响
    if (charsRemoved != charsAdded) {
        if (startLine > 1) m_changedLines.insert(startLine - 1);
        m_changedLines.insert(endLine + 1);
    }
}

// 将字符位置转换为行号（1-based）
int DiffRenderer::positionToLineNumber(int position)
{
    QTextCursor cursor(m_document);
    cursor.setPosition(position);
    return cursor.blockNumber() + 1; // QTextCursor行号是0-based，转为1-based
}
