#ifndef FILEENCODINGDETECTOR_H
#define FILEENCODINGDETECTOR_H

#include <QObject>
#include <QString>

// 文件编码自动检测工具类
class FileEncodingDetector : public QObject
{
    Q_OBJECT
public:
    explicit FileEncodingDetector(QObject *parent = nullptr);

    // 检测文件编码（支持UTF-8、UTF-8 with BOM、GBK、GB2312）
    // 返回编码名称（如"UTF-8"、"GBK"），失败返回默认编码"UTF-8"
    QString detectFileEncoding(const QString &filePath);

private:
    bool isUtf8(const QByteArray &data);
    bool isUtf8WithBom(const QByteArray &data);
    bool isGbkOrGb2312(const QByteArray &data);
    bool isUtf16LE(const QByteArray &data);
    bool isUtf16BE(const QByteArray &data);
};

#endif // FILEENCODINGDETECTOR_H
