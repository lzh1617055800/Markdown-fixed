#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QObject>
#include <QStringList>

class AppSettings : public QObject
{
    Q_OBJECT
public:
    explicit AppSettings(QObject *parent = nullptr);

    // 扫描临时目录，返回所有自动保存文件（按修改时间排序，最新的在前）
    QStringList scanAutoSaveFiles(const QString &tempDir);
    // 保存最近文件列表到系统配置
    void saveRecentFiles(const QStringList &recentFiles);
    // 从系统配置加载最近文件列表
    QStringList loadRecentFiles();
private:
    const QString m_recentFilesKey = "RecentFiles";
};

#endif // APPSETTINGS_H
