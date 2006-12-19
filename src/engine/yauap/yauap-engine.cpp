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


#include <qprocess.h>

/* FIXME use the qt bindings.... 
   we can't use the glib signal handling because it requires a g_main loop 
   otherwise the glib bindings might probably be ok because gstreamer is using glib anyway
   Currently the engine uses the c bindings for signal handling and the glib bindings for function calls

*/
#include <dbus/dbus-glib.h>
#include <klocale.h>
#include <iostream>

#define DBUS_API_SUBJECT_TO_CHANGE
#include <dbus/connection.h>

//#define MANUAL_YAUAP_START
#define YAUAP_STARTUP_TIMEOUT 10000

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
        engine->update_metadata();
    else if(dbus_message_is_signal( msg, "org.yauap.CommandInterface", "EosSignal"))
       	engine->track_ended();
    else
        handled = false;

    return (handled ? DBUS_HANDLER_RESULT_HANDLED : DBUS_HANDLER_RESULT_NOT_YET_HANDLED);
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


DBusConnection::DBusConnection( void* c )
{
    qt_connection = NULL;
    dbus_connection = NULL;
    context = c;
}

DBusConnection::~DBusConnection()
{
    close();
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
    GError *error = NULL;
    /* make sure we really stopped */
    stop();

    /* quit the player */
    if ( !dbus_g_proxy_call( remote_object, "quit", &error,
                          G_TYPE_INVALID,
                          G_TYPE_INVALID) )
    {

                debug() << "quit failed " << error->message <<  endl;
                g_error_free( error );
    }


    /* destroy Qt DBus connection */
    if(con)
        delete con; 

    /* free remote object */
    if(remote_object)
        g_object_unref(remote_object);

    /* kill yauap */
#ifndef MANUAL_YAUAP_START
    helper.kill();
#endif
}

/* fetch metadata from yauap */
void 
yauapEngine::update_metadata(void){
    Engine::SimpleMetaBundle bndl;
    GError *error = NULL;
    char **reply_list;
    char **reply_ptr;
    debug() << " emit metadata change " << endl;


    if( !dbus_g_proxy_call (remote_object, "get_metadata", &error,
        G_TYPE_INVALID,G_TYPE_STRV, &reply_list, G_TYPE_INVALID) )
    {
        debug() << "get_metadata failed " << error->message <<  endl;
        g_error_free(error);
        return;
    }
    for(reply_ptr = reply_list; *reply_ptr; reply_ptr++)
    {
#define ASSIGN(a,b)  if(!strncmp(*reply_ptr,b,strlen(b)) && strlen(*reply_ptr + strlen(b) + 1)){ \
                         bndl.a = *reply_ptr + strlen(b) + 1; \
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
    /* free reply_list */
    for( reply_ptr = reply_list; *reply_ptr; reply_ptr++)
        free( *reply_ptr );
    free( reply_list );

    debug() << "title:" << bndl.title << endl;
    debug() << "artist:" << bndl.artist << endl;
    debug() << "album:" << bndl.album << endl;
    debug() << "comment:" << bndl.comment << endl;
    debug() << "genre:" << bndl.genre << endl;
    debug() << "samplerate:" << bndl.samplerate << endl;
    debug() << "year:" << bndl.year << endl;
    debug() << "tracknr:" << bndl.tracknr << endl;
    debug() << "length:" << bndl.length << endl;
    debug() << "bitrate:" << bndl.bitrate << endl;


    /* do not overwrite manually generated metadata from audio cds */
    if(bndl.title.isEmpty() && loaded_url.protocol() == "cdda")
        return;

    emit EngineBase::metaData( bndl );
}


/* fetch current sample buffer from yauap */
const Engine::Scope &
yauapEngine::scope(){ 
    GError *error = NULL;
    GArray *arr;
    int i;

//        debug() << " update scope " << endl;

    if (!dbus_g_proxy_call(remote_object, "get_scopedata", &error,
        G_TYPE_INVALID,DBUS_TYPE_G_UCHAR_ARRAY, &arr, G_TYPE_INVALID))
    {
        debug() << "get_scopedata failed " << error->message <<  endl;
        g_error_free( error );
        return m_scope;
    }
    /* 2 channel 16 bit samples */
    if(arr->len == SCOPESIZE * 2)
    {
        /* and voila about 1000 memcpys later the data reaches amarok */
        gint16 *data = reinterpret_cast<gint16 *>( arr->data );
        for( i=0; i < SCOPESIZE ; i++)
            m_scope[i] = data[i];
    }else
       debug() << "get_scopedata returned the wrong amount of data " << arr->len << endl;
    g_array_free( arr, TRUE);
    return m_scope;
}


/* tell amarok that the current track ended */
void 
yauapEngine::track_ended( void )
{
    m_state = Engine::Idle;
    emit trackEnded();
}


/* init engine */
bool 
yauapEngine::init( void )
{
    GError *error = NULL;
    int i,  ret = 0;
      
    debug() << "In init" << endl;

#ifndef MANUAL_YAUAP_START
    /* start yauap in slave mode */
    helper.addArgument( "yauap" );
    helper.addArgument( "-noexit" );
    helper.setCommunication( QProcess::Stdin|QProcess::Stdout );
       
    if( !helper.start() )
    {
        debug() << "could not start yauap " << endl;
        emit statusText( i18n( "could not start yauap" ) );
        return false;
    }
#endif

	
    /* create and open qt DBus connection so that we are able to receive signals */
    con = new DBusConnection( (void*) this );
    con->open();

    /* init g type for the DBus glib bindings */
    g_type_init();

    /* connect to DBus */
    bus = dbus_g_bus_get( DBUS_BUS_SESSION, &error);
    if( !bus )
    {
        debug() << "could not connect to dbus" << endl;
        emit statusText( i18n( "Error: couldn't connect to dbus" ) );
        return false;
    }

    /* create a yauap Remote Object */
    remote_object = dbus_g_proxy_new_for_name( bus,
                                             "org.yauap.CommandService",
                                             "/yauapObject",
                                             "org.yauap.CommandInterface");
    if( !remote_object )
    {
        debug() << "could not create remote object" << endl;
        return false;
    }

    /* makes sure the player is stopped: retry the call a few times because it takes some
    time until yauap registered its dbus service  */

    for( i=0; i < YAUAP_STARTUP_TIMEOUT ; i++ ){
        if( dbus_g_proxy_call(remote_object, "stop", &error,
            G_TYPE_INVALID,G_TYPE_INT,&ret, G_TYPE_INVALID) )
             break;
//                debug() << "stop failed " << error->message <<  endl;
        usleep(1000);
        g_error_free( error );
        error = NULL;
    }
    if( i >= YAUAP_STARTUP_TIMEOUT )
    {
        debug() << "timed out waiting for yauap" << endl;
        emit statusText( i18n( "Error: timed out waiting for yauap" ) );

        return false;
    }

    return true;
}

/* check if the given url can be decoded */
bool 
yauapEngine::canDecode( const KURL &kurl ) const 
{
    GError *error = NULL;
    QString qurl = kurl.prettyURL(); 
    const char* url = qurl.ascii();
    int can_decode = 0;
    
    debug() << "In canDecode " << url << endl ;
    if (!dbus_g_proxy_call( remote_object, "can_decode", &error,
                          G_TYPE_STRING,url,
                          G_TYPE_INVALID,
                          G_TYPE_INT,&can_decode,
                          G_TYPE_INVALID))
    {

        debug() << "canDecode " << error->message <<  endl;
        g_error_free( error );
        return false;
    }
       
    debug() << "=> " << can_decode << endl;

    if( can_decode )
        return true;
    return false;
}

/* load a new track FIXME: implement cross fading */
bool 
yauapEngine::load( const KURL &url, bool isStream )
{
    GError *error = NULL;
    QString qurl = url.prettyURL();
    const char* curl = qurl.ascii();
    int gerror = 0;
    debug() << "In load " << curl << endl;

    /* check if the url can really be decoded */
    if( !canDecode( url ) )
    {
        debug() << "cannot decode this file" << endl;
        g_error_free( error );
        return false;
    }

    m_isStream = isStream;

    Engine::Base::load( url, isStream || url.protocol() == "http" );
    change_state(Engine::Idle);

    if (!dbus_g_proxy_call( remote_object, "load", &error,
        G_TYPE_STRING,curl,
        G_TYPE_INVALID,
        G_TYPE_INT,&gerror,
        G_TYPE_INVALID))
    {
        debug() << "load failed " << error->message <<  endl;
        g_error_free( error );
        return false;
    }

    debug() << "=> " << gerror << endl;
    if( !gerror )
        return false;
    loaded_url = url;
    return true;
}

/* set volume */
void 
yauapEngine::setVolumeSW( uint volume )
{
    GError *error = NULL;
    int gerror = 0;
    debug() << "In setVolumeSW " << volume << endl;
    if( !dbus_g_proxy_call( remote_object, "set_volume", &error,
        G_TYPE_UINT,volume,G_TYPE_INVALID,G_TYPE_INT,&gerror,G_TYPE_INVALID))
    {
        debug() << "set_volume failed " << error->message <<  endl;
        g_error_free(error);
        return;
    }
    debug() << "=> " << gerror << endl;
}

/* start playback */
bool 
yauapEngine::play( uint offset )
{
    GError *error = NULL;
    int gerror = 0;
    debug() << "In play" << endl;
    if (!dbus_g_proxy_call(remote_object, "start", &error,
        G_TYPE_UINT,offset,G_TYPE_INVALID,G_TYPE_INT,&gerror,G_TYPE_INVALID))
    {
        debug() << "play failed " << error->message <<  endl;
        g_error_free( error );
        return false;
    }

    debug() << "=> " << gerror << endl;
    if(gerror)
    {
        change_state( Engine::Playing );
        return true;
    }
    change_state( Engine::Empty );
    return false;
}


/* stop playback */
void 
yauapEngine::stop( void )
{
    GError *error = NULL;
    int gerror = 0;
    change_state( Engine::Empty );

    if (!dbus_g_proxy_call(remote_object, "stop", &error,
        G_TYPE_INVALID,G_TYPE_INT,&gerror,G_TYPE_INVALID))
    {
        debug() << "stop failed " << error->message <<  endl;
        g_error_free(error);
        return;
    }

    debug() << "=> " << gerror << endl;
}


/* pause playback */
void 
yauapEngine::pause( void )
{
    GError *error = NULL;
    int gerror = 0;
    debug() << "In pause " << endl;
    if ( !dbus_g_proxy_call(remote_object, "pause", &error,
        G_TYPE_INVALID,G_TYPE_INT,&gerror,G_TYPE_INVALID) )
    {
        debug() << "pause failed " << error->message <<  endl;
        g_error_free( error );
        return;
    }

    debug() << "=> " << gerror << endl;

    if( gerror ){
        if( state() == Engine::Playing )
            change_state( Engine::Paused );
         else
            change_state( Engine::Playing );
    }
}

/* unpause playback */
void 
yauapEngine::unpause( void )
{
    pause();
}

/* get track length in ms */
uint 
yauapEngine::length( void ) const 
{
    GError *error = NULL;
    uint length = 0;
    debug() << "In length " << endl;
    if (!dbus_g_proxy_call(remote_object, "get_length", &error,
        G_TYPE_INVALID,G_TYPE_UINT,&length,G_TYPE_INVALID))
    {
        debug() << "get_length failed " << error->message <<  endl;
        g_error_free(error);
        return 0;
    }

    debug() << "=> " << length << endl;
    return length;
}

/* get current position */
uint 
yauapEngine::position( void ) const 
{
    GError *error = NULL;
    uint position = 0;
//	debug() << "In position " << endl;
    if ( !dbus_g_proxy_call(remote_object, "get_position", &error,
        G_TYPE_INVALID,G_TYPE_UINT,&position,G_TYPE_INVALID) )
    {

        debug() << "get_positon failed " << error->message <<  endl;
        g_error_free(error);
        return 0;
    }
//	debug() << "=> " << position << endl;
    return position;
}

/* seek to offset in ms */
void 
yauapEngine::seek( uint offset )
{
    GError *error = NULL;
    int ret = 0;
    debug() << "In seek " << endl;
    if ( !dbus_g_proxy_call( remote_object, "seek", &error,
        G_TYPE_UINT, offset, G_TYPE_INVALID, G_TYPE_INT, &ret, G_TYPE_INVALID) )
    {
        debug() << "seek failed " << error->message <<  endl;
        g_error_free(error);
        return;
    }
    debug() << "=> " << ret << endl;
}


bool
yauapEngine::getAudioCDContents(const QString &device, KURL::List &urls)
{
    GError *error = NULL;
    char **reply_list;
    char **reply_ptr;
    int i = 0;
    

    debug() << "Getting AudioCD contents..." << endl;

    if( !dbus_g_proxy_call (remote_object, "get_audio_cd_contents", &error,
        G_TYPE_STRING,(char*)device.latin1(),G_TYPE_INVALID,G_TYPE_STRV, &reply_list, G_TYPE_INVALID) )
    {
        debug() << "get_audio_cd_contents failed " << error->message <<  endl;
        g_error_free(error);
        return false;
    }
   
    cd_tracks.clear(); 
    for(reply_ptr = reply_list; *reply_ptr; reply_ptr++)
    {
        Engine::SimpleMetaBundle b;
        char* saveptr;
        KURL url = QString("cdda://").append( strtok_r(*reply_ptr,"=",&saveptr)); 
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
    /* free reply_list */
    for( reply_ptr = reply_list; *reply_ptr; reply_ptr++)
        free( *reply_ptr );
    free( reply_list );

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

