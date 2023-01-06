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


#include "localstorage.h"
#include <QLoggingCategory>
Q_LOGGING_CATEGORY(log_localStorage, "LocalStorage")
#define sDebug() qCDebug(log_localStorage)
#define sInfo() qCInfo(log_localStorage)

QDir LocalStorage::rootDir;
QString LocalStorage::rootPath;

LocalStorage::LocalStorage()
{

}

bool LocalStorage::isUsable()
{
    return !rootPath.isEmpty();
}

void LocalStorage::setRootPath(const QString arootDir)
{
    sDebug() << "Root path is " << arootDir;
    rootDir.setPath(arootDir);
    rootPath = QDir::cleanPath(rootDir.absolutePath());
    rootDir.setFilter(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
}


QList<LocalStorage::FileInfo> LocalStorage::list(const QString path)
{
    sDebug() << "Request listing " << path;
    rootDir.refresh();
    QList<LocalStorage::FileInfo> toret;

    if (violateRoot(path))
        return toret;
    QString nPath = rootDir.absoluteFilePath(rootPath + "/" + path);
    nPath = QDir::cleanPath(nPath);
    //sDebug() << nPath;
    QFileInfo nfi(nPath);
    if (!nfi.isDir())
        return toret;
    QDir nDir(nPath);
    sDebug() << "Listing" << nDir.absolutePath();
    nDir.refresh();
    auto infoList = nDir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotDot);
    for (const QFileInfo& info : qAsConst(infoList))
    {
        //sDebug() << info;
        LocalStorage::FileInfo fi;
        fi.name = info.fileName();
        if (info.isDir())
            fi.type = FileType::Dir;
        if (info.isFile())
            fi.type = FileType::File;
        toret.append(fi);
    }
    FileInfo fi;
    if (rootDir.absolutePath() != nDir.absolutePath())
    {
        fi.type = FileType::Dir;
        fi.name = "..";
        toret.append(fi);
    }
    return toret;
}

QByteArray LocalStorage::getFile(const QString path)
{
    sDebug() << "Access to " << path;
    if (violateRoot(path))
        return QByteArray();
    QFile file(rootPath + "/" + path);
    if (!file.exists() || QFileInfo(rootPath + "/" + path).isDir())
        return QByteArray();
    file.open(QIODevice::ReadOnly);
    return file.readAll();
}

bool LocalStorage::createFile(const QString path, const QByteArray& data)
{
    sDebug() << "Wanting to create " << path << "Bytes : " << data.size();
    if (violateRoot(path))
        return false;
    QFile file(rootPath + "/" + path);
    if (!file.open(QIODevice::WriteOnly))
        return false;
    file.write(data);
    file.close();
    return true;
}

QFile* LocalStorage::prepareWriteFile(const QString path, const unsigned int size)
{
    sDebug() << "Wanting to create " << path;
    if (violateRoot(path))
        return new QFile();
    QFile* toret = new QFile(rootPath + "/" + path);
    toret->open(QIODevice::WriteOnly);
    return toret;
}

bool LocalStorage::remove(const QString path)
{
    sDebug() << "Removing " << path;
    if (violateRoot(path))
        return false;
    return QFile::remove(rootPath + "/" + path);
}

bool LocalStorage::makeDir(const QString path)
{
    sDebug() << "Creating directory " << path;
    if (violateRoot(path))
        return false;
    return rootDir.mkdir(rootPath + "/" + path);
}

bool LocalStorage::rename(const QString path, const QString newPath)
{
    if (violateRoot(path) || violateRoot(newPath))
        return false;
    return rootDir.rename(path, newPath);
}

bool LocalStorage::violateRoot(QString toCheck)
{
    QString path = rootDir.absoluteFilePath(rootPath + "/" + toCheck);
    path = QDir::cleanPath(path);
    sDebug() << "Testing violation root, root : " << rootPath;
    sDebug() << "                        path : " << path;
    if (path.indexOf(rootPath) == -1)
        return true;
    return false;
}
