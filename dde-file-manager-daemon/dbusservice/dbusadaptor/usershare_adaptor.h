/*
 * This file was generated by qdbusxml2cpp version 0.8
 * Command line was: qdbusxml2cpp -i usershare/usersharemanager.h -c UserShareAdaptor -l UserShareManager -a dbusadaptor/usershare_adaptor usershare.xml
 *
 * qdbusxml2cpp is Copyright (C) 2016 The Qt Company Ltd.
 *
 * This is an auto-generated file.
 * This file may have been hand-edited. Look for HAND-EDIT comments
 * before re-generating it.
 */

#ifndef USERSHARE_ADAPTOR_H
#define USERSHARE_ADAPTOR_H

#include <QtCore/QObject>
#include <QtDBus/QtDBus>
#include "usershare/usersharemanager.h"
QT_BEGIN_NAMESPACE
class QByteArray;
template<class T> class QList;
template<class Key, class Value> class QMap;
class QString;
class QStringList;
class QVariant;
QT_END_NAMESPACE

/*
 * Adaptor class for interface com.deepin.filemanager.daemon.UserShareManager
 */
class UserShareAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.deepin.filemanager.daemon.UserShareManager")
    Q_CLASSINFO("D-Bus Introspection", ""
"  <interface name=\"com.deepin.filemanager.daemon.UserShareManager\">\n"
"    <method name=\"setUserSharePassword\">\n"
"      <arg direction=\"in\" type=\"s\" name=\"username\"/>\n"
"      <arg direction=\"in\" type=\"s\" name=\"passward\"/>\n"
"      <arg direction=\"out\" type=\"b\" name=\"result\"/>\n"
"    </method>\n"
"    <method name=\"addGroup\">\n"
"      <arg direction=\"in\" type=\"s\" name=\"groupName\"/>\n"
"      <arg direction=\"out\" type=\"b\" name=\"result\"/>\n"
"    </method>\n"
"  </interface>\n"
        "")
public:
    UserShareAdaptor(UserShareManager *parent);
    virtual ~UserShareAdaptor();

    inline UserShareManager *parent() const
    { return static_cast<UserShareManager *>(QObject::parent()); }

public: // PROPERTIES
public Q_SLOTS: // METHODS
    bool addGroup(const QString &groupName);
    bool setUserSharePassword(const QString &username, const QString &passward);
Q_SIGNALS: // SIGNALS
};

#endif
