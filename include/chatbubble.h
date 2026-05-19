#ifndef CHATBUBBLE_H
#define CHATBUBBLE_H
#include <QWidget>
#include <QTimer>
#include <QLabel>

class ChatBubble : public QWidget
{
    Q_OBJECT
public:
    enum Role{User,Assistant};
    enum State{Waiting,Streaming,Done};

    explicit ChatBubble(Role role,QWidget *parent = nullptr);
    void setText(const QString &text);
    void appendText(const QString &chunk);
    void setState(State state);

    State state()const{return m_state;}
    Role role()const{return m_role;}
    QString text()const{return m_fullText;}

private:
    void updateStyle();

    State m_state;
    Role m_role;
    QString m_fullText;

    QLabel *m_label;
    QTimer *m_waitingTimer;
    int m_waitingDots;
};

#endif // CHATBUBBLE_H
