#include "FileEncodingDetector.h"
#include <QFile>
#include <QByteArray>

FileEncodingDetector::FileEncodingDetector(QObject *parent)
    : QObject(parent)
{
}

// 核心函数：检测文件编码
QString FileEncodingDetector::detectFileEncoding(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return "UTF-8"; // 打开失败默认返回UTF-8
    }

    // 读取前4KB数据用于检测（足够判断编码）
    QByteArray data = file.read(4096);
    file.close();

    // 优先检测BOM
    if (isUtf8WithBom(data)) {
        return "UTF-8";
    }
    if (isUtf16LE(data)) {
        return "UTF-16LE"; // 修复Bug 6：精确返回大小端
    }
    if (isUtf16BE(data)) {
        return "UTF-16BE";
    }

    // 检测UTF-8（无BOM）
    if (isUtf8(data)) {
        return "UTF-8";
    }

    // 检测GBK/GB2312（中文Windows默认编码，修复Bug 7）
    if (isGbkOrGb2312(data)) {
        return "GBK";
    }

    // 所有检测失败，返回系统默认编码
    return "System";
}

// 检测是否为UTF-8编码（无BOM）
bool FileEncodingDetector::isUtf8(const QByteArray &data)
{
    int bytesLeft = 0; // 剩余需要验证的字节数
    for (char c : data) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (bytesLeft == 0) {
            if ((uc & 0x80) == 0) {
                continue; // 单字节字符（0xxxxxxx）
            } else if ((uc & 0xE0) == 0xC0) {
                bytesLeft = 1; // 双字节字符（110xxxxx）
            } else if ((uc & 0xF0) == 0xE0) {
                bytesLeft = 2; // 三字节字符（1110xxxx）
            } else if ((uc & 0xF8) == 0xF0) {
                bytesLeft = 3; // 四字节字符（11110xxx）
            } else {
                return false; // 无效的UTF-8起始字节
            }
        } else {
            if ((uc & 0xC0) != 0x80) {
                return false; // 非后续字节（10xxxxxx）
            }
            bytesLeft--;
        }
    }
    return bytesLeft == 0; // 所有多字节序列完整
}

// 检测是否为UTF-8 with BOM（开头有0xEF 0xBB 0xBF）
bool FileEncodingDetector::isUtf8WithBom(const QByteArray &data)
{
    return data.size() >= 3 &&
           static_cast<unsigned char>(data[0]) == 0xEF &&
           static_cast<unsigned char>(data[1]) == 0xBB &&
           static_cast<unsigned char>(data[2]) == 0xBF;
}

// 检测UTF-16 Little Endian BOM (FF FE)
bool FileEncodingDetector::isUtf16LE(const QByteArray &data)
{
    return data.size() >= 2 &&
           static_cast<unsigned char>(data[0]) == 0xFF &&
           static_cast<unsigned char>(data[1]) == 0xFE;
}

// 检测UTF-16 Big Endian BOM (FE FF)
bool FileEncodingDetector::isUtf16BE(const QByteArray &data)
{
    return data.size() >= 2 &&
           static_cast<unsigned char>(data[0]) == 0xFE &&
           static_cast<unsigned char>(data[1]) == 0xFF;
}

// 检测是否为GBK/GB2312编码（通过常见中文字符范围判断）
bool FileEncodingDetector::isGbkOrGb2312(const QByteArray &data)
{
    int chineseCharCount = 0; // 中文字符计数
    int totalCharCount = 0;   // 总字符计数

    for (int i = 0; i < data.size(); ) {
        unsigned char uc = static_cast<unsigned char>(data[i]);
        if (uc <= 0x7F) {
            // 单字节字符（ASCII）
            totalCharCount++;
            i++;
        } else {
            // 双字节字符（可能是GBK/GB2312）
            if (i + 1 < data.size()) {
                unsigned char uc2 = static_cast<unsigned char>(data[i+1]);
                // GBK编码范围：0x81-0xFE（首字节），0x40-0xFE（次字节，除0x7F）
                if ((uc >= 0x81 && uc <= 0xFE) &&
                    (uc2 >= 0x40 && uc2 <= 0xFE && uc2 != 0x7F)) {
                    chineseCharCount++;
                    totalCharCount++;
                    i += 2;
                    continue;
                }
            }
            // 不符合GBK规则，直接返回false
            return false;
        }
    }

    // 中文占比超过30%则判定为GBK/GB2312
    return totalCharCount > 0 && (chineseCharCount * 100 / totalCharCount) > 30;
}
