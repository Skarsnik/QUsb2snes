#pragma once

#include <QSettings>
#include <QStringView>
#include <qmetaobject.h>
#include <QDebug>

class Settings : private QSettings
{
    Q_OBJECT
public:
    enum SettingsV {
        RemoteHost
    };
    Q_ENUM(SettingsV)
private:
    struct SettingEntry {
        SettingsV value;
        QStringView entry;
        QMetaType::Type type;
    };
    constexpr static SettingEntry settingsEntries[1] = {
        {RemoteHost, u"remotehost", QMetaType::QString}
    };

public:
#ifndef Q_OS_WIN
    explicit Settings(QObject *parent = nullptr) : QSettings("nyo.fr", "QUsb2Snes") {}
#else
    explicit Settings(QObject *parent = nullptr) : QSettings("config.ini", QSettings::IniFormat) {};
#endif
    ~Settings() {}
    template<SettingsV key> auto value()
    {
        constexpr QMetaType::Type type = [&]() {
            for (const SettingEntry& mapping : settingsEntries)
                if (mapping.value == key)
                    return mapping.type;
            return QMetaType::Void;
        }();
        QStringView settingString = [&]() {
            for (const SettingEntry& mapping : settingsEntries)
                if (mapping.value == key)
                    return mapping.entry;
            return QStringView();
            const QMetaEnum meta = QMetaEnum::fromType<SettingsV>();
            const QString text = QString::fromLatin1(meta.valueToKey(key));
            return QStringView(text);
        }();

        qDebug() << "Value() StringKey"  << settingString;
        if constexpr (type == QMetaType::Bool) {
            return QSettings::value(settingString.toString()).toBool();
        } else if constexpr (type == QMetaType::Int) {
            return QSettings::value(settingString.toString()).toInt();
        } else if constexpr (type == QMetaType::Double) {
            return QSettings::value(settingString.toString()).toDouble();
        } else if constexpr (type == QMetaType::QString) {
            return QSettings::value(settingString.toString()).toString();
        } else {
            return QSettings::value(settingString.toString());
        }
    }
    /*void    setValue(SettingsV key, const QVariant& value)
    {
        QStringView keyString;
        for (const SettingEntry& mapping : settingsEntries)
            if (mapping.value == key)
                keyString = mapping.entry;
        if (keyString.isEmpty())
        {
            const QMetaEnum meta = QMetaEnum::fromType<SettingsV>();
            const QString text = QString::fromLatin1(meta.valueToKey(key));
            keyString = text;
        }
        qDebug() << "SetValue() StringKey " << keyString;
        QSettings::setValue(QString(keyString), value);
    }*/
    template<SettingsV key, typename T> void setValue(T value)
    {
        constexpr QMetaType::Type type = [&]() {
            for (const SettingEntry& mapping : settingsEntries)
                if (mapping.value == key)
                    return mapping.type;
            return QMetaType::Void;
        }();
        QStringView settingString = [&]() {
            for (const SettingEntry& mapping : settingsEntries)
                if (mapping.value == key)
                    return mapping.entry;
            const QMetaEnum meta = QMetaEnum::fromType<SettingsV>();
            const QString text = QString::fromLatin1(meta.valueToKey(key));
            return QStringView(text);
        }();
        constexpr bool canConvert = (type == QMetaType::Int && std::is_same<T, int>::value)
                                    || (type == QMetaType::QString && std::is_convertible<T, QString>::value)
                                    || (type == QMetaType::Bool && std::is_same<T, bool>::value)
                                    || (type == QMetaType::Double && std::is_same<T, double>::value);

        #if __cpp_static_assert >= 202306L
            const QMetaEnum meta = QMetaEnum::fromType<SettingsV>();
            const QString text = QString::fromLatin1(meta.valueToKey(key));
            static_assert(canConvert, std::format("Settings: Unexpected type used for the key. {0} is a {1}", text, QMetaType::typeName(type)));
        #else
            static_assert(canConvert, "Settings: Unexpected type used for the key");
        #endif
        QSettings::setValue(settingString.toString(), value);
        return ;
    }

};
