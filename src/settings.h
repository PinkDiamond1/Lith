#ifndef SETTINGS_H
#define SETTINGS_H

#include "common.h"
#include <QSettings>
#include <QDebug>

#define SETTING(type, name, ...) \
    PROPERTY_NOSETTER(type, name, __VA_ARGS__) \
    public: \
        void name ## Set (const type &o) { \
            if (m_ ## name != o) { \
                m_ ## name = o; \
                QSettings s; \
                s.setValue(STRINGIFY(name), o); \
                emit name ## Changed(); \
                s.sync(); \
            } \
        }

/*
 * USAGE:
 * just add a SETTING to the class header and it'll get exposed to
 * QML and C++ code through the Settings singleton object
 */
class Settings : public QObject {
    Q_OBJECT
    SETTING(int, lastOpenBuffer, -1)
    SETTING(qreal, baseFontSize, 10)
    SETTING(bool, showFullLink, true)
    SETTING(bool, showSendButton, false)
    SETTING(QString, host)
    SETTING(int, port, 9001)
    SETTING(bool, encrypted, true)
    SETTING(QString, passphrase)
public:
    Settings(QObject *parent = nullptr);
private:
    QSettings m_settings;
};

#endif // SETTINGS_H