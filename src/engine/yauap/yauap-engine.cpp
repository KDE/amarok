/***************************************************************************
                       yauap-engine.h - yauap engine plugin

copyright            : (C) 2006 by Sascha Sommer <saschasommer@freenet.de>
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qapplication.h>

#include <klocale.h>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/connection.h>

//#define MANUAL_YAUAP_START
#define YAUAP_DBUS_SERVICE "org.yauap.CommandService"
#define YAUAP_DBUS_PATH "/yauapObject"
#define YAUAP_DBUS_INTERFACE "org.yauap.CommandInterface"

#include "yauap-engine.h"
#include "debug.h"



AMAROK_EXPORT_PLUGIN( yauapEngine )

/* signal handler for DBus signals */
static DBusHandlerResult 
signal_handler( DBusConnection * /*con*/, DBusMessage *msg, void *data )
{
    yauapEngine* engine = (yauapEngine*)data;
    const char *objectpath  = dbus_message_get_path(msg);
    const char *member      = dbus_message_get_member(msg);
    const char *interface   = dbus_message_get_interface(msg);
    bool        handled     = true;

    debug() << "SIGNAL member " << member << " interface " << interface  << " objpath " << objectpath << endl;

    if (dbus_message_is_signal( msg, "org.yauap.CommandInterface", "MetadataSignal")) 
       QApplication::postEvent(engine, new QCustomEvent(3004));
    else if(dbus_message_is_signal( msg, "org.yauap.CommandInterface", "EosSignal")) {
        if (engine->m_state == Engine::Playing)
            QApplication::postEvent(engine, new QCustomEvent(3000));
    }
    else if(dbus_message_is_signal( msg, "org.yauap.CommandInterface", "ErrorSignal"))
    {
        char* text = NULL;
        DBusError error;
        dbus_error_init(&error);
        if(dbus_message_get_args( msg, &error, DBUS_TYPE_STRING, &text, DBUS_TYPE_INVALID)) {
            QCustomEvent* e = new QCustomEvent(3002);
            e->setData(new QString(text));
            QApplication::postEvent(engine, e);
        }
    }
    else
        handled = false;

    return (handled ? DBUS_HANDLER_RESULT_HANDLED : DBUS_HANDLER_RESULT_NOT_YET_HANDLED);
}

int
yauapProcess::commSetupDoneC()
{
    int r = Amarok::Process::commSetupDoneC();
    int fd = open("/dev/null", O_RDWR);
    dup2(fd, 1);
    dup2(fd, 2);
    close(fd);
    return r;
}

/* create a qt dbus connection that will receive the signals */
bool 
DBusConnection::open()
{
    DBusError error;
    dbus_error_init( &error );

    debug() << " connecting to dbus" << endl;

    // close open connection
    close();

    /* connect to session bus */
    dbus_connection = dbus_bus_get_private( DBUS_BUS_SESSION, &error );  /* dbus_bus_get somehow doesn't work here */
    if( dbus_error_is_set(&error) ) 
    {
        debug() << "unable to connect to DBUS." << endl;
        dbus_error_free(&error);
        return false;
    }
    dbus_connection_set_exit_on_disconnect( dbus_connection, false );

    /* create qt connection */
    qt_connection = new DBusQt::Connection( this );
    qt_connection->dbus_connection_setup_with_qt_main( dbus_connection );

    if ( !dbus_connection_add_filter(dbus_connection, signal_handler, context, NULL) )
    {
        debug() << "Failed to add filter function." << endl;
        return false;
    }

    /* Match for DBUS_INTERFACE_DBUS */
    dbus_bus_add_match( dbus_connection, "type='signal',interface='org.yauap.CommandInterface'", &error);
    if ( dbus_error_is_set( &error ) ) 
    {
        debug() << "Error adding match, " << error.name << " " << error.message;
        dbus_error_free (&error);
        return false;
    }

    debug() << " connected " << endl;
    return true;
}

/* close qt dbus connection */
void 
DBusConnection::close()
{
    debug() << "close DBusConnection" << endl;

    if( dbus_connection )
        dbus_connection_close( dbus_connection );

    if(qt_connection)
        qt_connection->close();

    /* DBusConnection::open () calls dbus_bus_get (), we need to unref the connection */
    debug() << "calling dbus connection close" << endl;

    dbus_connection = NULL;
    qt_connection = NULL;
    debug() << "DBusConnection closed" << endl;
}


DBusConnection::DBusConnection( yauapEngine* c )
{
    qt_connection = NULL;
    dbus_connection = NULL;
    context = c;
}

DBusConnection::~DBusConnection()
{
    close();
}

bool
DBusConnection::send(const char *method, int first_arg_type, ...)
{
    dbus_uint32_t serial = 0;
    bool ret = false;

    QMutexLocker lock(&m_mutex);

    DBusMessage* msg = dbus_message_new_method_call(
            YAUAP_DBUS_SERVICE, YAUAP_DBUS_PATH, YAUAP_DBUS_INTERFACE,
            method);

    if (msg) {
        va_list ap;

        va_start(ap, first_arg_type);
        dbus_message_append_args_valist(msg, first_arg_type, ap);
        va_end(ap);

        ret = dbus_connection_send(dbus_connection, msg, &serial);
        dbus_message_unref (msg);
    }

    return ret;
}

int
DBusConnection::call(const char *method, int first_arg_type, ...)
{
    dbus_uint32_t ret = -1;

    va_list ap;
    va_start (ap, first_arg_type);
    DBusMessage* msg = send_with_reply(method, first_arg_type, ap);
    va_end (ap);

    if (msg) {
        DBusMessageIter args;
        if (dbus_message_iter_init(msg, &args) && 
                (DBUS_TYPE_INT32 == dbus_message_iter_get_arg_type(&args) ||
                 DBUS_TYPE_UINT32 == dbus_message_iter_get_arg_type(&args)))
            dbus_message_iter_get_basic(&args, &ret);

        dbus_message_unref (msg);
    }

    return ret;
}

DBusMessage*
DBusConnection::send_with_reply(const char* method, int first_arg_type, ...)
{
    va_list ap;
    va_start(ap, first_arg_type);
    DBusMessage* msg = send_with_reply(method, first_arg_type, ap);
    va_end(ap);
    return msg;
}

DBusMessage*
DBusConnection::send_with_reply(const char* method, int first_arg_type, va_list ap)
{
    QMutexLocker lock(&m_mutex);

    DBusMessage* msg = dbus_message_new_method_call(
            YAUAP_DBUS_SERVICE, YAUAP_DBUS_PATH, YAUAP_DBUS_INTERFACE, method);

    if (msg) {
        DBusError error;
        dbus_error_init(&error);

        dbus_message_append_args_valist(msg, first_arg_type, ap);

        DBusMessage* oldmsg = msg;
        msg = dbus_connection_send_with_reply_and_block(dbus_connection, oldmsg, -1, &error);
        DBusDispatchStatus status;
        while ((status = dbus_connection_get_dispatch_status(dbus_connection)) == DBUS_DISPATCH_DATA_REMAINS)
            dbus_connection_dispatch (dbus_connection);
        dbus_message_unref (oldmsg);

        if (!msg)
            debug() << "dbus error while waiting for reply: " << error.message << endl;
    }

    return msg;
}


/* emit state change signal */
void
yauapEngine::change_state( Engine::State state )
{
    m_state = state;
    emit stateChanged(m_state);
}


/* destroy engine */
yauapEngine::~yauapEngine()
{
    /* make sure we really stopped */
    stop();

    /* quit the player */
    if ( !con->send("quit", DBUS_TYPE_INVALID) )
        debug() << "quit failed " <<  endl;

    delete con;
}

/* fetch metadata from yauap */
void 
yauapEngine::update_metadata()
{
    Engine::SimpleMetaBundle* bndl = new Engine::SimpleMetaBundle;
    debug() << " emit metadata change " << endl;

    DBusMessage* msg = con->send_with_reply("get_metadata", DBUS_TYPE_INVALID);
    if (msg) {
        DBusMessageIter args;
        if (dbus_message_iter_init(msg, &args) && DBUS_TYPE_ARRAY ==
                dbus_message_iter_get_arg_type(&args)) {
            DBusMessageIter sub;
            dbus_message_iter_recurse(&args, &sub);
            dbus_message_iter_next(&args);
            while (dbus_message_iter_get_arg_type(&sub) == DBUS_TYPE_STRING) {
                char* reply_ptr = 0;
                dbus_message_iter_get_basic(&sub, &reply_ptr);
                dbus_message_iter_next(&sub);

                debug() << "reply_ptr: " << reply_ptr << endl;

#define ASSIGN(a,b)  if(!strncmp(reply_ptr,b,strlen(b)) && strlen(reply_ptr + strlen(b) + 1)){ \
                         bndl->a = reply_ptr + strlen(b) + 1; \
                         continue; \
                     }

                ASSIGN( title, "title" )
                ASSIGN( artist, "artist" )
                ASSIGN( album, "album" )
                ASSIGN( comment, "comment" )
                ASSIGN( genre, "genre" )
                ASSIGN( samplerate, "samplerate" )
                ASSIGN( year, "date" )
                ASSIGN( tracknr, "track-number" )
                ASSIGN( length, "length" )
                ASSIGN( bitrate, "bitrate" )
#undef ASSIGN
            }
        }
        dbus_message_unref(msg);
    }

    debug() << "title:" << bndl->title << endl;
    debug() << "artist:" << bndl->artist << endl;
    debug() << "album:" << bndl->album << endl;
    debug() << "comment:" << bndl->comment << endl;
    debug() << "genre:" << bndl->genre << endl;
    debug() << "samplerate:" << bndl->samplerate << endl;
    debug() << "year:" << bndl->year << endl;
    debug() << "tracknr:" << bndl->tracknr << endl;
    debug() << "length:" << bndl->length << endl;
    debug() << "bitrate:" << bndl->bitrate << endl;


    /* do not overwrite manually generated metadata from audio cds */
    if(bndl->title.isEmpty() && loaded_url.protocol() == "cdda")
        return;

    QCustomEvent* e = new QCustomEvent(3003);
    e->setData(bndl);
    QApplication::postEvent(this, e);
}


/* fetch current sample buffer from yauap */
const Engine::Scope &
yauapEngine::scope()
{
    int len = 0;
    dbus_int16_t* data = 0;

    DBusMessage* msg = con->send_with_reply("get_scopedata", DBUS_TYPE_INVALID);
    if (msg) {
        DBusMessageIter args;
        if (dbus_message_iter_init(msg, &args) && DBUS_TYPE_ARRAY ==
                dbus_message_iter_get_arg_type(&args)) {
            DBusMessageIter sub;
            dbus_message_iter_recurse(&args, &sub);
            dbus_message_iter_next(&args);
            dbus_message_iter_get_fixed_array(&sub,&data,&len);
        }
        dbus_message_unref(msg);
    }

    /* 2 channel 16 bit samples */
    if(len == SCOPESIZE * 2)
    {
        for( int i=0; i < SCOPESIZE ; i++)
            m_scope[i] = data[i];
    }else
       debug() << "get_scopedata returned the wrong amount of data " << len << endl;

    return m_scope;
}

void
yauapEngine::yauapProcessExited()
{
    debug() << "yauapProcessExited!!!!!" << endl;

    closeDbusConnection();
    initDbusConnection();
}

void
yauapEngine::closeDbusConnection()
{
    /* destroy Qt DBus connection */
    delete con;
    con = 0;

    /* kill yauap */
#ifndef MANUAL_YAUAP_START
    helper.kill();
#endif
}

bool
yauapEngine::initDbusConnection()
{
#ifndef MANUAL_YAUAP_START
    /* start yauap in slave mode */
    helper.clearArguments();
    helper << "yauap" << "-noexit";

    if( !helper.start(KProcess::NotifyOnExit, KProcess::All))
    {
        debug() << "could not start yauap " << endl;
        emit statusText( i18n( "could not start yauap" ) );
        return false;
    }
#endif

    /* create and open qt DBus connection so that we are able to receive signals */
    con = new DBusConnection( this );
    if (!con->open())
    {
        debug() << "could not connect to dbus" << endl;
        emit statusText( i18n( "Error: could not connect to dbus" ) );
        return false;
    }

    /* makes sure the player is stopped: retry the call a few times because it takes some
    time until yauap registered its dbus service  */
    for( int i=0; i < 2; ++i ) {
        if( con->send("stop", DBUS_TYPE_INVALID) >= 0 )
            return true;
        usleep(20 * 1000);
    }

    return false;
}

#if 0
void 
yauapEngine::handleDbusError(const char* method)
{
    debug() << method << " failed "  <<  endl;

    closeDbusConnection();
    initDbusConnection();
}
#endif

void
yauapEngine::customEvent(QCustomEvent *e)
{
    QString* message = static_cast<QString*>(e->data());

    switch (e->type()) {
    case 3000:
        m_state = Engine::Idle;
        emit trackEnded();
        break;
    case 3002:
        emit statusText(*message);
        delete message;
        break;
    case 3003:
        {
            Engine::SimpleMetaBundle* bundle = static_cast<Engine::SimpleMetaBundle*>(e->data());
            emit metaData(*bundle);
            delete bundle;
            break;
        }
    case 3004:
        update_metadata();
        break;
    default:
        ;
    }
}

/* init engine */
bool 
yauapEngine::init()
{
    debug() << "In init" << endl;

    m_state = Engine::Idle;

    connect(&helper, SIGNAL(processExited(KProcess*)), SLOT(yauapProcessExited()));

    if (initDbusConnection())
        return true;

    emit statusText( i18n( "Error: timed out waiting for yauap" ) );
    return false;
}

/* check if the given url can be decoded */
bool
yauapEngine::canDecode( const KURL &kurl ) const
{
    QCString qurl = kurl.url().utf8();
    const char* url = qurl.data();

    return con->call("can_decode", DBUS_TYPE_STRING, &url, DBUS_TYPE_INVALID) > 0;
}

/* load a new track FIXME: implement cross fading */
bool 
yauapEngine::load( const KURL &url, bool isStream )
{
    QString qurl = url.url();
    const char* curl = qurl.ascii();

    m_isStream = isStream;

    Engine::Base::load( url, isStream || url.protocol() == "http" );
    change_state(Engine::Idle);

    if (!curl || !con->call("load", DBUS_TYPE_STRING, &curl, DBUS_TYPE_INVALID))
        return false;

    loaded_url = url;
    return true;
}

/* set volume */
void 
yauapEngine::setVolumeSW( uint volume )
{
    dbus_uint32_t dbus_volume = volume;
    int ret;

    debug() << "In setVolumeSW " << volume << endl;

    ret = con->send("set_volume", DBUS_TYPE_UINT32, &dbus_volume, DBUS_TYPE_INVALID);
    debug() << "=> " << ret << endl;
}

/* start playback */
bool 
yauapEngine::play( uint offset )
{
    dbus_uint32_t dbus_offset = offset;

    debug() << "In play" << endl;

    if (con->send("start", DBUS_TYPE_UINT32, &dbus_offset, DBUS_TYPE_INVALID) > 0)
    {
        change_state( Engine::Playing );
        return true;
    }

    change_state( Engine::Empty );
    return false;
}

/* stop playback */
void 
yauapEngine::stop()
{
    change_state( Engine::Empty );

    if (con->send("stop", DBUS_TYPE_INVALID) <= 0)
    {
        debug() << "stop failed " <<  endl;
        return;
    }

    change_state(Engine::Empty);
}


/* pause playback */
void 
yauapEngine::pause()
{
    debug() << "In pause " << endl;

    if (!con->call("pause", DBUS_TYPE_INVALID))
        return;

    if( state() == Engine::Playing )
        change_state( Engine::Paused );
    else
        change_state( Engine::Playing );
}

/* unpause playback */
void
yauapEngine::unpause()
{
    pause();
}

/* get track length in ms */
uint
yauapEngine::length() const
{
    debug() << "In length " << endl;

    int length = con->call("get_length", DBUS_TYPE_INVALID);
    if (length < 0) return 0;

    debug() << "length is => " << length << endl;
    return (uint) length;
}

/* get current position */
uint
yauapEngine::position() const
{
    int position = 0;

    position = con->call("get_position", DBUS_TYPE_INVALID);

    if (position < 0) position = 0;
    return (uint) position;
}

/* seek to offset in ms */
void
yauapEngine::seek( uint offset )
{
    dbus_uint32_t dbus_offset = offset;

    if (!con->send("seek", DBUS_TYPE_UINT32, &dbus_offset, DBUS_TYPE_INVALID))
        debug() << "seek failed " <<  endl;
}

bool
yauapEngine::getAudioCDContents(const QString &device, KURL::List &urls)
{
    debug() << "Getting AudioCD contents..." << endl;

    QCString cdevice = device.latin1();
    const char* cdevice_ptr = cdevice.data();

    DBusMessage* msg = con->send_with_reply("get_audio_cd_contents",
            DBUS_TYPE_STRING, &cdevice_ptr, DBUS_TYPE_INVALID);
    if (msg) {
        DBusMessageIter args;
        int i = 0;

        if (dbus_message_iter_init(msg, &args) && DBUS_TYPE_ARRAY ==
                dbus_message_iter_get_arg_type(&args)) {
            DBusMessageIter sub;
            dbus_message_iter_recurse(&args, &sub);
            dbus_message_iter_next(&args);
            while (dbus_message_iter_get_arg_type(&sub) == DBUS_TYPE_STRING) {
                char* reply_ptr = 0;
                dbus_message_iter_get_basic(&sub, &reply_ptr);
                dbus_message_iter_next(&sub);

                debug() << "reply_ptr: " << reply_ptr << endl;

                Engine::SimpleMetaBundle b;
                char* saveptr;
                KURL url = QString("cdda://").append( strtok_r(reply_ptr,"=",&saveptr)); 
                urls << url;
                debug() << url << endl;
                b.title  = QString( i18n( "Track %1" ) ).arg( i+1 );
                b.length = strtok_r(NULL,"=",&saveptr);
                b.album = "AudioCD";
                b.tracknr = i+1;
                b.samplerate = "44100";
                b.bitrate = "1411";
                cd_tracks.push_back(b);
                ++i;
            }
        }
        dbus_message_unref(msg);
    }

    return true;
}

bool 
yauapEngine::metaDataForUrl(const KURL &url, Engine::SimpleMetaBundle &b)
{
    if ( url.protocol() == "cdda" )
    {
         b = cd_tracks[url.host().toUInt()-1];
         return true;
    }
    return false;
}

#include "yauap-engine.moc"
