#include "AppSettings.h"
#include <QDir>
#include <QFileInfoList>
#include <QDateTime>
#include <QSettings>

AppSettings::AppSettings(QObject *parent)
    : QObject(parent)
{
}

// 扫描临时目录中的自动保存文件
QStringList AppSettings::scanAutoSaveFiles(const QString &tempDir)
{
    QDir dir(tempDir);
    if (!dir.exists()) return {};

    // 筛选自动保存文件（AutoSave_*.md）
    QStringList nameFilters;
    nameFilters << "AutoSave_*.md";
    dir.setNameFilters(nameFilters);

    // 获取文件信息列表，按修改时间降序排序（最新的在前）
    QFileInfoList fileInfos = dir.entryInfoList(
        QDir::Files,
        QDir::Time  // 默认降序排列（最新的在前）
        );

    // 提取文件路径
    QStringList filePaths;
    for (const QFileInfo &info : fileInfos) {
        filePaths << info.filePath();
    }
    return filePaths;
}
void AppSettings::saveRecentFiles(const QStringList &recentFiles)
{
    // QSettings会自动选择系统配置位置：
    // Windows：注册表HKEY_CURRENT_USER\Software\公司名\应用名
    // Linux：~/.config/公司名/应用名.conf
    // macOS：~/Library/Preferences/公司名.应用名.plist
    QSettings settings("LightMark", "LightMark Editor");
    settings.setValue(m_recentFilesKey, recentFiles);
}

// 从系统配置加载最近文件列表
QStringList AppSettings::loadRecentFiles()
{
    QSettings settings("LightMark", "LightMark Editor");
    return settings.value(m_recentFilesKey, QStringList()).toStringList();
}
