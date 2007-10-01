/***************************************************************************
                       yauap-engine.h - yauap engine plugin

copyright            : (C) 2006 by Sascha Sommer <ssommer@suse.de>
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_YAUAP_ENGINE_H
#define AMAROK_YAUAP_ENGINE_H

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/connection.h>

#include <amarok.h>

#include "enginebase.h"
#include "debug.h"

class yauapEngine;

class DBusConnection : public QObject
{
    friend class yauapEngine;

    DBusQt::Connection *qt_connection;
    DBusConnection *dbus_connection;
    yauapEngine *context;
    QMutex m_mutex;

public:
    bool open();
    void close();
    DBusConnection( yauapEngine *context );
    ~DBusConnection();

    bool send(const char *method, int first_arg_type, ...);
    DBusMessage* send_with_reply(const char* method, int first_arg_type, ...);
    DBusMessage* send_with_reply(const char* method, int first_arg_type, va_list);
    int call(const char *method, int first_arg_type, ...);
};

class yauapProcess : public Amarok::Process
{
public:
    yauapProcess(QObject* parent) : Amarok::Process(parent) {}

    virtual int commSetupDoneC();
};

static DBusHandlerResult signal_handler( DBusConnection *, DBusMessage *, void *);

class yauapEngine : public Engine::Base
{
    Q_OBJECT

    friend class DBusConnection;

    friend DBusHandlerResult signal_handler( DBusConnection *, DBusMessage *, void *);

    virtual ~yauapEngine();
    virtual bool init();
    virtual bool canDecode( const KURL& ) const;
    virtual uint position() const ;
    virtual bool load( const KURL&, bool );
    virtual bool play( uint );
    virtual void stop();
    virtual void pause();
    virtual void unpause();
    virtual void setVolumeSW( uint );
    virtual void seek( uint );
    virtual uint length() const ;
    virtual Engine::State state() const { return m_state; }
    virtual const Engine::Scope &scope();
    virtual bool getAudioCDContents(const QString &device, KURL::List &urls);
    virtual bool metaDataForUrl(const KURL &url, Engine::SimpleMetaBundle &b);
public:
    yauapEngine() : EngineBase(), helper(0) {}
    /* these need to be public because they are called from the dbus signal handler */
    void update_metadata();
    void update_scope();
    virtual void customEvent(QCustomEvent*);

private slots:
    void yauapProcessExited();

private:
    KURL loaded_url;
    std::vector<Engine::SimpleMetaBundle> cd_tracks;
    void change_state( Engine::State );
    bool initDbusConnection();
    void closeDbusConnection();

    Engine::State m_state;
    DBusConnection *con;
    /* helper process to start */
    yauapProcess helper;
};

#endif
