#pragma once

#include <QSettings>
#include <qmetaobject.h>

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
    explicit Settings(QObject *parent = nullptr) : QSettings("Test.ini", QSettings::IniFormat) {};
    template<SettingsV key> auto value()
    {
        constexpr QMetaType::Type type = [&]() {
            for (const SettingEntry& mapping : settingsEntries)
                if (mapping.value == key)
                    return mapping.type;
            return QMetaType::Void;
        }();
        constexpr QStringView settingString = [&]() {
            for (const SettingEntry& mapping : settingsEntries)
                if (mapping.value == key)
                    return mapping.entry;
            const QMetaEnum meta = QMetaEnum::fromType<SettingsV>();
            const QString text = QString::fromLatin1(meta.valueToKey(key));
            return QStringView(text);
        }();
        qDebug() << "Value() StringKey"  << settingString;
        if constexpr (type == QMetaType::Bool) {
            return QSettings::value(settingString).toBool();
        } else if constexpr (type == QMetaType::Int) {
            return QSettings::value(settingString).toInt();
        } else if constexpr (type == QMetaType::Double) {
            return QSettings::value(settingString).toDouble();
        } else if constexpr (type == QMetaType::QString) {
            return QSettings::value(settingString).toString();
        } else {
            return QSettings::value(settingString);
        }
    }
    void    setValue(SettingsV key, const QVariant& value)
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
        QSettings::setValue(keyString, value);
    }
    template<SettingsV key, typename T> void setValue(T value)
    {
        constexpr QMetaType::Type type = [&]() {
            for (const SettingEntry& mapping : settingsEntries)
                if (mapping.value == key)
                    return mapping.type;
            return QMetaType::Void;
        }();
        constexpr QStringView settingString = [&]() {
            for (const SettingEntry& mapping : settingsEntries)
                if (mapping.value == key)
                    return mapping.entry;
            const QMetaEnum meta = QMetaEnum::fromType<SettingsV>();
            const QString text = QString::fromLatin1(meta.valueToKey(key));
            return QStringView(text);
        }();
        constexpr bool canConvert = type == QMetaType::Int && std::is_same<T, int>::value
                                    || type == QMetaType::QString && std::is_convertible<T, QString>::value
                                    || type == QMetaType::Bool && std::is_same<T, bool>::value
                                    || type == QMetaType::Double && std::is_same<T, double>::value;

        #if __cpp_static_assert >= 202306L
            const QMetaEnum meta = QMetaEnum::fromType<SettingsV>();
            const QString text = QString::fromLatin1(meta.valueToKey(key));
            static_assert(canConvert, std::format("Settings: Unexpected type used for the key. {0} is a {1}", text, QMetaType::typeName(type)));
        #else
            static_assert(canConvert, "Settings: Unexpected type used for the key");
        #endif
        QSettings::setValue(settingString, value);
        return ;
    }

};
