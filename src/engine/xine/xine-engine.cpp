//Copyright: (C) 2004 Max Howell, <max.howell@methylblue.com>
//Copyright: (C) 2003-2004 J. Kofler, <kaffeine@gmx.net>

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "xine-config.h"
#include "xine-engine.h"
#include "xine-scope.h"

AMAROK_EXPORT_PLUGIN( XineEngine )

#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <math.h>
#include <qapplication.h>
#include <qdir.h>


//define this to use xine in a more standard way
//#define XINE_SAFE_MODE


#ifdef NDEBUG
static inline kndbgstream debug() { return kndbgstream(); }
#else
static inline kdbgstream  debug() { return kdbgstream( "[xine-engine] ", 0, 0 ); }
#endif


//some logging static globals
namespace Log
{
    static uint bufferCount = 0;
    static uint scopeCallCount = 1; //prevent divideByZero
    static uint noSuitableBuffer = 0;
};


XineEngine::XineEngine()
  : EngineBase()
  , m_xine( 0 )
  , m_stream( 0 )
  , m_audioPort( 0 )
  , m_eventQueue( 0 )
  , m_post( 0 )
{
    myList->next = myList; //init the buffer list

    addPluginProperty( "StreamingMode",  "NoStreaming" );
    addPluginProperty( "HasConfigure",   "true" );
}

XineEngine::~XineEngine()
{
    if( m_stream && xine_get_status( m_stream ) == XINE_STATUS_PLAY ) {
       for( int v = xine_get_param( m_stream, XINE_PARAM_AUDIO_AMP_LEVEL ) - 1; v >= 0; v-- ) {
          xine_set_param( m_stream, XINE_PARAM_AUDIO_AMP_LEVEL, v );
          int sleep = 13000 * (-log10( v + 1 ) + 2);

          ::usleep( sleep );
       }
       xine_stop( m_stream );
    }

    if (m_stream)     xine_close(m_stream);
    if (m_eventQueue) xine_event_dispose_queue(m_eventQueue);
    if (m_stream)     xine_dispose(m_stream);
    if (m_audioPort)  xine_close_audio_driver(m_xine, m_audioPort);
    if (m_post)       xine_post_dispose(m_xine, m_post);
    if (m_xine)       xine_exit(m_xine);

    debug() << "xine closed\n";

    debug() << "Scope statistics:\n"
            << "  Average list size: " << Log::bufferCount / Log::scopeCallCount << endl
            << "  Buffer failure:    " << double(Log::noSuitableBuffer*100) / Log::scopeCallCount << "%\n";
}

bool
XineEngine::init()
{
    debug() <<
        "Initialising..\n"
        "Please report bugs to http://bugs.kde.org\n"
        "Please note that some bugs can be fixed by using the newest available xine-lib\n"
        #ifdef XINE_SAFE_MODE
        "Running in safe-mode\n"
        #endif
        "Build stamp: " << __DATE__ << ' ' << __TIME__ << endl;

    m_xine = xine_new();

    if( !m_xine )
    {
        KMessageBox::error( 0, i18n("amaroK could not initialise xine.") );
        return false;
    }

    #ifdef XINE_SAFE_MODE
    xine_engine_set_param( m_xine, XINE_ENGINE_PARAM_VERBOSITY, 99 );
    #endif

    QString
    path  = QDir::homeDirPath();
    path += "/.%1/config";
    path  = QFile::exists( path.arg( "kaffeine" ) ) ? path.arg( "kaffeine" ) : path.arg( "xine" );

    debug() << "Using configuration: " << path << endl;
    xine_config_load( m_xine, QFile::encodeName( path ) );

    xine_init( m_xine );

    m_audioPort = xine_open_audio_driver( m_xine, "auto", NULL );
    if( !m_audioPort )
    {
        KMessageBox::error( 0, i18n("xine was unable to initialize any audio-drivers.") );
        return false;
    }

    m_stream = xine_stream_new( m_xine, m_audioPort, NULL );
    if( !m_stream )
    {
        KMessageBox::error( 0, i18n("amaroK could not create a new xine-stream.") );
        return false;
    }

    #ifndef XINE_SAFE_MODE
    //implemented in xine-scope.h
    m_post = scope_plugin_new( m_xine, m_audioPort );

    //less buffering, faster seeking.. TODO test
    xine_set_param( m_stream, XINE_PARAM_METRONOM_PREBUFFER, 6000 );
    xine_set_param( m_stream, XINE_PARAM_IGNORE_VIDEO, 1 );
    //xine_trick_mode( m_stream, XINE_TRICK_MODE_SEEK_TO_TIME, 1 );
    #endif

    xine_event_create_listener_thread( m_eventQueue = xine_event_new_queue( m_stream ),
                                       &XineEngine::XineEventListener,
                                       (void*)this );

    #ifndef XINE_SAFE_MODE
    startTimer( 200 ); //prunes the scope
    #endif

    return true;
}

bool
XineEngine::load( const KURL &url, bool stream )
{
    if( XINE_VERSION == "1-rc6a" && url.protocol() == "http" ) {
       KMessageBox::sorry( 0, i18n( "Sorry xine 1-rc6a cannot play remote streams, please upgrade to 1-rc7" ) );
       return false;
    }


    Engine::Base::load( url, stream || url.protocol() == "http" );

    if( xine_open( m_stream, url.url().local8Bit() ) )
    {
       #ifndef XINE_SAFE_MODE
       //we must ensure the scope is pruned of old buffers
       timerEvent( 0 );

       xine_post_out_t *source = xine_get_audio_source( m_stream );
       xine_post_in_t  *target = (xine_post_in_t*)xine_post_input( m_post, const_cast<char*>("audio in") );
       xine_post_wire( source, target );
       #endif

       return true;
    }

    return false;
}

bool
XineEngine::play( uint offset )
{
    if( xine_play( m_stream, 0, offset ) )
    {
        emit stateChanged( Engine::Playing );

        return true;
    }

    emit stateChanged( Engine::Empty );

    switch( xine_get_error( m_stream ) )
    {
    case XINE_ERROR_NO_INPUT_PLUGIN:
        KMessageBox::error( 0, i18n("No input plugin available; check your installation.") );
        break;
    case XINE_ERROR_NO_DEMUX_PLUGIN:
        KMessageBox::error( 0, i18n("No demux plugin available; check your installation.") );
        break;
    case XINE_ERROR_DEMUX_FAILED:
        KMessageBox::error( 0, i18n("Demuxing failed; check your installation.") );
        break;
    default:
        KMessageBox::error( 0, i18n("Internal error; check your installation.") );
        break;
    }

    xine_close( m_stream );

    return false;
}

void
XineEngine::stop()
{
    m_url = KURL(); //to ensure we return Empty from state()

    std::fill( m_scope.begin(), m_scope.end(), 0 );

    xine_stop( m_stream );
    xine_set_param( m_stream, XINE_PARAM_AUDIO_CLOSE_DEVICE, 1);

    emit stateChanged( Engine::Empty );
}

void
XineEngine::pause()
{
    if( xine_get_param( m_stream, XINE_PARAM_SPEED ) )
    {
        xine_set_param( m_stream, XINE_PARAM_SPEED, XINE_SPEED_PAUSE );
        xine_set_param( m_stream, XINE_PARAM_AUDIO_CLOSE_DEVICE, 1);
        emit stateChanged( Engine::Paused );

    } else {

        xine_set_param( m_stream, XINE_PARAM_SPEED, XINE_SPEED_NORMAL );
        emit stateChanged( Engine::Playing );
    }
}

Engine::State
XineEngine::state() const
{
    switch( xine_get_status( m_stream ) )
    {
    case XINE_STATUS_PLAY: return xine_get_param( m_stream, XINE_PARAM_SPEED ) ? Engine::Playing : Engine::Paused;
    case XINE_STATUS_IDLE: return Engine::Empty;
    case XINE_STATUS_STOP:
    default:               return m_url.isEmpty() ? Engine::Empty : Engine::Idle;
    }
}

uint
XineEngine::position() const
{
    int pos;
    int time = 0;
    int length;

    xine_get_pos_length( m_stream, &pos, &time, &length );

    return time;
}

void
XineEngine::seek( uint ms )
{
    xine_play( m_stream, 0, (int)ms );
}

void
XineEngine::setVolumeSW( uint vol )
{
    xine_set_param( m_stream, XINE_PARAM_AUDIO_AMP_LEVEL, vol );
}

bool
XineEngine::canDecode( const KURL &url ) const
{
    //TODO should free the file_extensions char*
    static QStringList list = QStringList::split( ' ', xine_get_file_extensions( m_xine ) );

    //TODO proper mimetype checking

    const QString path = url.path();
    const QString ext  = path.mid( path.findRev( '.' ) + 1 ).lower();
    return ext != "txt" && list.contains( ext );
}

static int64_t current_vpts;

const Engine::Scope&
XineEngine::scope()
{
    if( xine_get_status( m_stream ) != XINE_STATUS_PLAY )
       return m_scope;

    //prune the buffer list and update the current_vpts timestamp
    timerEvent( 0 );

    for( int frame = 0; frame < 512; )
    {
        MyNode *best_node = 0;

        for( MyNode *node = myList->next; node != myList; node = node->next, Log::bufferCount++ )
            if( node->vpts <= current_vpts && (!best_node || node->vpts > best_node->vpts) )
               best_node = node;

        if( !best_node || best_node->vpts_end < current_vpts ) {
           Log::noSuitableBuffer++; break; }

        int64_t
        diff  = current_vpts;
        diff -= best_node->vpts;
        diff *= 1<<16;
        diff /= myMetronom->pts_per_smpls;

        const int16_t*
        data16  = best_node->mem;
        data16 += diff;

        diff /= myChannels;

        int
        n  = best_node->num_frames;
        n -= diff;
        n += frame; //clipping for # of frames we need
        if( n > 512 ) n = 512; //bounds limiting

        for( int a, c; frame < n; ++frame, data16 += myChannels ) {
            for( a = c = 0; c < myChannels; ++c )
                a += data16[c];

            a /= myChannels;
            m_scope[frame] = a;
        }

        current_vpts = best_node->vpts_end;
        current_vpts++; //FIXME needs to be done for some reason, or you get situations where it uses same buffer again and again
    }

    Log::scopeCallCount++;

    return m_scope;
}

void
XineEngine::timerEvent( QTimerEvent* )
{
   //here we prune the buffer list regularly

   //we operate on a subset of the list for thread-safety
   MyNode * const first_node = myList->next;
   MyNode const * const list_end = myList;

   current_vpts = (xine_get_status( m_stream ) == XINE_STATUS_PLAY)
      ? xine_get_current_vpts( m_stream )
      : std::numeric_limits<int64_t>::max(); //if state is not playing OR paused, empty the list

   for( MyNode *prev = first_node, *node = first_node->next; node != list_end; node = node->next )
   {
      //we never delete first_node
      //this maintains thread-safety
      if( node->vpts_end < current_vpts ) {
         prev->next = node->next;

         free( node->mem );
         free( node );

         node = prev;
      }

      prev = node;
   }
}

amaroK::PluginConfig*
XineEngine::configure() const
{
    return new XineConfigDialog( m_xine );
}

void
XineEngine::customEvent( QCustomEvent *e )
{
    switch( e->type() )
    {
    case 3000:
        emit trackEnded();
        break;

    case 3001:
        #define message static_cast<QString*>(e->data())
        KMessageBox::error( 0, (*message).arg( m_url.prettyURL() ) );
        delete message;
        break;

    case 3002:
        emit statusText( *message );
        delete message;
        #undef message
        break;

    default:
        ;
    }
}

void
XineEngine::XineEventListener( void *p, const xine_event_t* xineEvent )
{
    if( !p ) return;

    #define xe static_cast<XineEngine*>(p)

    switch( xineEvent->type )
    {
    case XINE_EVENT_UI_PLAYBACK_FINISHED:

        //emit signal from GUI thread
        QApplication::postEvent( xe, new QCustomEvent(3000) );
        break;

    case XINE_EVENT_PROGRESS:
    {
        xine_progress_data_t* pd = (xine_progress_data_t*)xineEvent->data;

        QString
        msg = "%1 %2%";
        msg = msg.arg( QString( pd->description ) )
                 .arg( KGlobal::locale()->formatNumber( pd->percent, 0 ) );

        QApplication::postEvent( xe, new QCustomEvent(QEvent::Type(3002), new QString(msg)) );

        break;
    }
    case XINE_EVENT_UI_MESSAGE:
    {
        debug() << "message received from xine\n";

        xine_ui_message_data_t *data = (xine_ui_message_data_t *)xineEvent->data;
        QString message;

        switch( data->type )
        {
        case XINE_MSG_NO_ERROR:
        {
            //series of \0 separated strings, terminated with a \0\0
            char str[2000];
            char *p = str;
            for( char *msg = data->messages; !(*msg == '\0' && *(msg+1) == '\0'); ++msg, ++p )
                *p = *msg == '\0' ? '\n' : *msg;
            *p = '\0';

            debug() << str << endl;

            break;
        }

        case XINE_MSG_ENCRYPTED_SOURCE:
            break;

        case XINE_MSG_UNKNOWN_HOST:
            message = i18n("The host is unknown for the URL: <i>%1</i>"); goto param;
        case XINE_MSG_UNKNOWN_DEVICE:
            message = i18n("The device name you specified seems invalid."); goto param;
        case XINE_MSG_NETWORK_UNREACHABLE:
            message = i18n("The network appears unreachable."); goto param;
        case XINE_MSG_AUDIO_OUT_UNAVAILABLE:
            message = i18n("Audio output unavailable; the device is busy."); goto param;
        case XINE_MSG_CONNECTION_REFUSED:
            message = i18n("The connection was refused for the URL: <i>%1</i>"); goto param;
        case XINE_MSG_FILE_NOT_FOUND:
            message = i18n("xine could not find the URL: <i>%1</i>"); goto param;
        case XINE_MSG_PERMISSION_ERROR:
            message = i18n("Access was denied for the URL: <i>%1</i>"); goto param;
        case XINE_MSG_READ_ERROR:
            message = i18n("The source cannot be read for the URL: <i>%1</i>"); goto param;
        case XINE_MSG_LIBRARY_LOAD_ERROR:
            message = i18n("A problem occurred while loading a library or decoder."); goto param;

        case XINE_MSG_GENERAL_WARNING:
            message = i18n("General Warning"); goto explain;
        case XINE_MSG_SECURITY:
            message = i18n("Security Warning"); goto explain;
        default:
            message = i18n("Unknown Error"); goto explain;


        explain:

            if(data->explanation)
            {
                message.prepend( "<b>" );
                message += "</b>:<p>";
                message += ((char *) data + data->explanation);
            }
            else break; //if no explanation then why bother!

            //FALL THROUGH

        param:

            message.prepend( "<p>" );
            message += "<p>";

            if(data->explanation)
            {
                message += "xine parameters: <i>";
                message += ((char *) data + data->parameters);
                message += "</i>";
            }
            else message += i18n("Sorry, no additional information is available.");

            QApplication::postEvent( xe, new QCustomEvent(QEvent::Type(3001), new QString(message)) );
        }

    } //case
    } //switch

    #undef xe
}

#include "xine-engine.moc"
