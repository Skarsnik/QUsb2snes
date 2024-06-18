#ifndef SQPATH_H
#define SQPATH_H

#include <QStandardPaths>
#include <QString>

#ifdef SQPROJECT_STANDALONE
#define __SQ_STANDALONE
#endif
#ifndef SQPROJECT_INSTALLED
#define __SQ_STANDALONE
#endif

namespace SQPath {
static const QString translationsPath()
{
#if defined(__SQ_STANDALONE) || defined(SQPROJECT_WINDOWS_INSTALL)
    return qApp->applicationDirPath() + "/i18n/";
#endif
#ifdef SQPROJECT_TRANSLATIONS_PATH
    return QString("%1/i18n/").arg(SQPROJECT_TRANSLATIONS_PATH);
#endif
    return "";
}

static const QString softwareDatasPath()
{
#if defined(__SQ_STANDALONE) || defined(SQPROJECT_WINDOWS_INSTALL)
    return qApp->applicationDirPath();
#endif
#ifdef SQPROJECT_UNIX_APP_SHARE
    return SQPROJECT_UNIX_APP_SHARE;
#endif
}

static const QString logDirectoryPath()
{
#ifdef __SQ_STANDALONE
    return qApp->applicationDirPath();
#endif
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
}

}

#endif // SQPATH_H
