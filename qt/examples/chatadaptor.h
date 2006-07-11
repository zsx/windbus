/*
 * This file was generated by dbusidl2cpp version 0.3
 * when processing input file /home/tjmaciei/src/kde4/playground/libs/qt-dbus/examples/com.trolltech.ChatInterface.xml
 *
 * dbusidl2cpp is Copyright (C) 2006 Trolltech AS. All rights reserved.
 *
 * This is an auto-generated file.
 */

#ifndef CHATADAPTOR_H_88051142890130
#define CHATADAPTOR_H_88051142890130

#include <QtCore/QObject>
#include <dbus/qdbus.h>
class QByteArray;
template<class T> class QList;
template<class Key, class Value> class QMap;
class QString;
class QStringList;
class QVariant;

/*
 * Adaptor class for interface com.trolltech.ChatInterface
 */
class ChatInterfaceAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.trolltech.ChatInterface")
public:
    ChatInterfaceAdaptor(QObject *parent);
    virtual ~ChatInterfaceAdaptor();

public: // PROPERTIES
public slots: // METHODS
signals: // SIGNALS
    void action(const QString &nickname, const QString &text);
    void message(const QString &nickname, const QString &text);
};

#endif
