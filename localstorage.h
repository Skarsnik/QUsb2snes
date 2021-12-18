#ifndef LOCALSTORAGE_H
#define LOCALSTORAGE_H

#include <QDir>



class LocalStorage
{
public:
    enum class FileType {
        Dir = 0,
        File = 1
    };

    struct FileInfo {
        QString     name;
        FileType    type;
    };

    static bool             isUsable();
    static void             setRootPath(const QString path);
    static QList<FileInfo>  list(const QString path);
    static QByteArray       getFile(const QString path);
    static bool             createFile(const QString path, const QByteArray& data);
    static QFile*           prepareWriteFile(const QString path, const unsigned int size);
    static bool             remove(const QString path);
    static bool             makeDir(const QString path);
    static bool             rename(const QString path, const QString newPath);

private:
    LocalStorage();

    static QDir    rootDir;
    static QString rootPath;
    static bool    violateRoot(QString toCheck);
};

#endif // LOCALSTORAGE_H
