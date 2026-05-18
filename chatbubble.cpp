#include <QHBoxLayout>
#include "chatbubble.h"

ChatBubble::ChatBubble(Role role,QWidget *parent)
    : QWidget(parent)
    , m_role(role)
    , m_state(Done)
    , m_waitingDots(0)
{
    m_label = new QLabel(this);
    m_label->setWordWrap(true);
    m_label->setTextFormat(Qt::PlainText);
    m_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_label->setMaximumWidth(500);

    m_waitingTimer = new QTimer(this);
    connect(m_waitingTimer,&QTimer::timeout,this,
    [this]()
    {
        m_waitingDots = (m_waitingDots + 1) % 4;
        m_label->setText(QString(m_waitingDots,'.'));
    });

    QHBoxLayout *outerLayout = new QHBoxLayout(this);
    outerLayout->setContentsMargins(8,4,8,4);

    if(m_role == User)
    {
        outerLayout->addStretch();
        outerLayout->addWidget(m_label);
    }else
    {
        outerLayout->addWidget(m_label);
        outerLayout->addStretch();
    }
    updateStyle();
}

void ChatBubble::setText(const QString &text)
{
    m_fullText = text;
    m_label->setText(text);
}

void ChatBubble::appendText(const QString &chunk)
{
    m_fullText += chunk;
    m_label->setText(m_fullText);
}

void ChatBubble::setState(State state)
{
    m_state = state;
    if(state == Waiting)
    {
        m_waitingTimer->start(400);
    }
    else
    {
        m_waitingTimer->stop();
        if(state == Done && m_fullText.isEmpty())
        {
            m_label->setText(QStringLiteral("(empty response)"));
        }
    }
    updateStyle();
}

void ChatBubble::updateStyle()
{
    QString bg,fg,border;
    if(m_role == User)
    {
        bg = QStringLiteral("#0078D4");
        fg = QStringLiteral("#FFFFFF");
        border = QStringLiteral("#0078D4");
    }
    else
    {
        bg = m_state == Waiting ? QStringLiteral("#F0F0F0")
                                : QStringLiteral("#F5F5F5");
        fg = QStringLiteral("#1A1A1A");
        border = QStringLiteral("#E0E0E0");
    }

    m_label->setStyleSheet(QString(
        "QLabel {"
        "  background-color: %1;"
        "  color:            %2;"
        "  border:           1px solid %3;"
        "  border-radius:    12px;"
        "  padding:          10px 14px;"
        "  font-size:        13px;"
        "}"
    ).arg(bg, fg, border));

}
