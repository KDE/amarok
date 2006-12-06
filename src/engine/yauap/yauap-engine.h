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

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/connection.h>


#include "enginebase.h"
#include "debug.h"



/* DBus Connection for the signal handler */
class DBusConnection : public QObject 
{
//    Q_OBJECT
    DBusQt::Connection *qt_connection;
    DBusConnection *dbus_connection;
    void *context;
public:
    bool open();
    void close();
    DBusConnection( void *context );
    ~DBusConnection();
};

class yauapEngine : public Engine::Base 
{
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
public: 
    yauapEngine() : EngineBase() {}
    /* these need to be public because they are called from the dbus signal handler */
    void update_metadata();
    void update_scope();
    void track_ended();
private:
    void change_state( Engine::State );
    DBusGConnection *bus;
    DBusGProxy *remote_object;
    Engine::State m_state;
    DBusConnection *con;
    /* helper process to start */
    QProcess helper;
};



