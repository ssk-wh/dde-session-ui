/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp com.deepin.dde.daemon.Launcher.xml -p launcherinter
 *
 * qdbusxml2cpp is Copyright (C) 2017 The Qt Company Ltd.
 *
 * This is an auto-generated file.
 * Do not edit! All changes made to it will be lost.
 */

#ifndef LAUNCHERINTER_H
#define LAUNCHERINTER_H

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>
#include <QtDBus/QtDBus>

#include "iteminfo.h"

/*
 * Proxy class for interface com.deepin.dde.daemon.Launcher
 */
class ComDeepinDdeDaemonLauncherInterface: public QDBusAbstractInterface
{
    Q_OBJECT
public:
    static inline const char *staticInterfaceName()
    { return "com.deepin.dde.daemon.Launcher"; }

public:
    ComDeepinDdeDaemonLauncherInterface(const QString &service, const QString &path, const QDBusConnection &connection, QObject *parent = nullptr);

    ~ComDeepinDdeDaemonLauncherInterface();

public Q_SLOTS: // METHODS
    inline QDBusPendingReply<ItemInfoList> GetAllItemInfos()
    {
        QList<QVariant> argumentList;
        return asyncCallWithArgumentList(QStringLiteral("GetAllItemInfos"), argumentList);
    }

Q_SIGNALS: // SIGNALS
    void NewAppLaunched(const QString &in0);
    void ItemChanged(const QString &status, ItemInfo itemInfo, qlonglong categoryID);
    void UninstallSuccess(const QString &in0);
};

namespace com {
  namespace deepin {
    namespace dde {
      namespace daemon {
        typedef ::ComDeepinDdeDaemonLauncherInterface Launcher;
      }
    }
  }
}
#endif
