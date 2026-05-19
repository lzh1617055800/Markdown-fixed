#ifndef DIFFRENDERER_H
#define DIFFRENDERER_H

#include <QObject>
#include <QTextDocument>
#include <QSet>

// 差分渲染管理器：计算文档变化的行，只重绘这些行
class DiffRenderer : public QObject
{
    Q_OBJECT
public:
    explicit DiffRenderer(QObject *parent = nullptr);

    // 设置需要监控的文档
    void setDocument(QTextDocument *doc);

    // 获取自上次以来变化的行号集合（1-based）
    QSet<int> getChangedLines();

    // 重置变化记录（用于重绘后清空）
    void resetChangedLines();

private slots:
    // 文档内容变化时触发，记录变化的行
    void onDocumentContentsChanged(int position, int charsRemoved, int charsAdded);

private:
    QTextDocument *m_document; // 监控的文档
    QSet<int> m_changedLines;  // 变化的行号集合

    // 辅助函数：将光标位置（字符索引）转换为行号（1-based）
    int positionToLineNumber(int position);
};

#endif // DIFFRENDERER_H
