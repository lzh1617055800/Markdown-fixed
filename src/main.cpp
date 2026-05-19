#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    // 创建QApplication对象（必须第一个创建）
    QApplication app(argc, argv);

    // 设置应用程序信息
    app.setApplicationName("Markdown Editor");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("MyCompany");

    // 创建主窗口
    MainWindow window;

    // 显示窗口
    window.show();

    // 进入事件循环，等待用户操作
    return app.exec();
}
