/*
 * Copyright (c) 2018 Sylvain "Skarsnik" Colinet.
 *
 * This file is part of the QUsb2Snes project.
 * (see https://github.com/Skarsnik/QUsb2snes).
 *
 * QUsb2Snes is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * QUsb2Snes is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with QUsb2Snes.  If not, see <https://www.gnu.org/licenses/>.
 */



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
    static bool             isInStorage(const QString path);
    static QString          storagePath(const QString path);

private:
    LocalStorage();

    static QDir    rootDir;
    static QString rootPath;
    static bool    violateRoot(QString toCheck);
};

#endif // LOCALSTORAGE_H
