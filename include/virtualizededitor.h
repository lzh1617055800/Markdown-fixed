#ifndef VIRTUALIZEDEDITOR_H
#define VIRTUALIZEDEDITOR_H

#include <QTextEdit>
#include <QScrollBar>

class VirtualizedEditor : public QTextEdit
{
    Q_OBJECT
public:
    explicit VirtualizedEditor(QWidget *parent = nullptr);

    QString getFullContent() const { return m_fullContent; }
    int getVisibleStartLine() const { return m_visibleStartLine; }
    int getVisibleEndLine() const { return m_visibleEndLine; }
    int getLinesPerView() const { return m_linesPerView; }
    void setFullContent(const QString &content);

protected:
    void paintEvent(QPaintEvent *event) override;
    void scrollContentsBy(int dx, int dy) override;

private:
    QString m_fullContent;
    QStringList m_fullLines;
    int m_visibleStartLine;
    int m_visibleEndLine;
    int m_linesPerView;
    int m_globalCursorLine;
    int m_globalCursorColumn;

    void updateVisibleLines();
    void rebuildFullDocument(); // 构造精确对齐的完整文档（含空行填充）
    void onTextChanged();

    int m_lastVisibleStart = -1;
    bool m_isProgrammaticChange = false;
};

#endif // VIRTUALIZEDEDITOR_H
