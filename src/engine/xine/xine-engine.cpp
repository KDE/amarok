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

#include "xine-engine.h"
#include "xine-scope.h"

AMAROK_EXPORT_PLUGIN( XineEngine )

#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <qdir.h>
#include <qtimer.h>

//xine headers have this as function parameter!
#define this this_
//need access to port_ticket
#define XINE_ENGINE_INTERNAL
    #include <xine/xine_internal.h>
    #include <xine/post.h>
#undef this


extern "C"
{
    post_class_t*
    scope_init_plugin( xine_t* );
}

static inline kdbgstream
debug()
{
    return kdbgstream( "[xine-engine] ", 0, 0 );
}

static inline void
emptyMyList()
{
    for( MyNode *next, *node = myList->next; node->next != myList; node = next )
    {
        next = node->next;

        free( node->buf.mem );
        free( node );
    }
    myList->next = myList;
}


//some logging static globals
namespace Log
{
    static uint buffNotFound = 0;
    static uint buffTooSmall = 0;
    static uint buffSize = 0;
    static uint listSize = 0;
    static uint scopeRequests = 0;
    static uint diffSize = 0;
    static uint buffCount = 0;
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
}

XineEngine::~XineEngine()
{
    if (m_stream)     xine_close(m_stream);
    if (m_eventQueue) xine_event_dispose_queue(m_eventQueue);
    if (m_stream)     xine_dispose(m_stream);
    if (m_audioPort)  xine_close_audio_driver(m_xine, m_audioPort);
    if (m_post)       xine_post_dispose(m_xine, m_post);
    if (m_xine)       xine_exit(m_xine);

    emptyMyList();

    debug() << "xine closed\n";

    debug() << "Scope statistics:\n"
            << "  Average buffer size: " << double(Log::buffSize) / Log::buffCount << endl
            << "  Average diff: " << double(Log::diffSize) / Log::scopeRequests << endl
            << "  Average list size: " << double(Log::listSize) / Log::scopeRequests << endl
            << "  Scope misses: " << Log::buffNotFound << endl
            << "  Times buffer too small: " << Log::buffTooSmall << endl;
}

bool
XineEngine::init()
{
    debug() << "Welcome to xine-engine! 9 out of 10 cats prefer xine!\n"
               "Please report bugs to amarok-devel@lists.sourceforge.net\n";

    m_xine = xine_new();

    if( !m_xine )
    {
        KMessageBox::error( 0, i18n("amaroK could not initialise xine.") );
        return false;
    }

    //xine_engine_set_param( m_xine, XINE_ENGINE_PARAM_VERBOSITY, 99 );


    QString
    path  = QDir::homeDirPath();
    path += "/.%1/config";
    path  = QFile::exists( path.arg( "kaffeine" ) ) ? path.arg( "kaffeine" ) : path.arg( "xine" );

    xine_config_load( m_xine, QFile::encodeName( path ) );

    xine_init( m_xine );

    m_audioPort = xine_open_audio_driver( m_xine, "auto", NULL );
    if( !m_audioPort )
    {
        KMessageBox::error( 0, i18n("xine was unable to initialize any audio-drivers.") );
        return false;
    }

    m_stream  = xine_stream_new( m_xine, m_audioPort, 0 );
    if( !m_stream )
    {
        KMessageBox::error( 0, i18n("amaroK could not create a new xine-stream.") );
        return false;
    }

    //less buffering, faster seeking.. TODO test
    xine_set_param( m_stream, XINE_PARAM_METRONOM_PREBUFFER, 6000 );
    //xine_trick_mode( m_stream, XINE_TRICK_MODE_SEEK_TO_TIME, 1 );


    m_eventQueue = xine_event_new_queue( m_stream );
    xine_event_create_listener_thread( m_eventQueue, &XineEngine::XineEventListener, (void*)this );


    //create scope post plugin
    //it syphons off audio buffers into our scope data structure

    post_class_t  *post_class  = scope_init_plugin( m_xine );
    post_plugin_t *post_plugin = post_class->open_plugin( post_class, 1, &m_audioPort, NULL );

    //code is straight from xine_init_post()
    //can't use that function as it only dlopens the plugins and our plugin is statically linked in

    post_plugin->running_ticket = m_xine->port_ticket;
    post_plugin->xine = m_xine;
    post_plugin->node = NULL;

    post_plugin->input_ids = (const char**)malloc(sizeof(char *)*2);
    xine_post_in_t *input = (xine_post_in_t *)xine_list_first_content( post_plugin->input );
    post_plugin->input_ids[0] = input->name; //"audio in";
    post_plugin->input_ids[1] = NULL;

    post_plugin->xine_post.type = PLUGIN_POST;

    post_plugin->output_ids = (const char**)malloc(sizeof(char *));
    post_plugin->output_ids[0] = NULL;

    m_post = &post_plugin->xine_post;

    post_class->dispose( post_class );


    startTimer( 200 ); //prunes the scope

    return true;
}

bool
XineEngine::load( const KURL &url, bool stream )
{
    Engine::Base::load( url, stream );

    xine_close( m_stream );

    return xine_open( m_stream, url.url().local8Bit() );
}

bool
XineEngine::play( uint offset )
{
    if( xine_play( m_stream, 0, offset ) )
    {
        xine_post_out_t *source = xine_get_audio_source( m_stream );
        xine_post_in_t  *target = (xine_post_in_t*)xine_post_input( m_post, const_cast<char*>("audio in") );

        xine_post_wire( source, target );

        emit stateChanged( Engine::Playing );

        return true;
    }

    emit stateChanged( Engine::Empty );

    //Xine will show a message via EventListener if there were problems
    return false;
}

void
XineEngine::stop()
{
    emptyMyList();

    xine_stop( m_stream );
    emit stateChanged( Engine::Empty );
}

void
XineEngine::pause()
{
    if( xine_get_param( m_stream, XINE_PARAM_SPEED ) )
    {
        xine_set_param( m_stream, XINE_PARAM_SPEED, XINE_SPEED_PAUSE );
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
    case XINE_STATUS_PLAY: return xine_get_param(m_stream, XINE_PARAM_SPEED) ? Engine::Playing : Engine::Paused;
    case XINE_STATUS_IDLE: return Engine::Idle;
    case XINE_STATUS_STOP:
    default:               return Engine::Empty;
    }
}

const Engine::Scope&
XineEngine::scope()
{
    if( xine_get_status( m_stream ) != XINE_STATUS_PLAY ) return m_scope;

    current_vpts = xine_get_current_vpts( m_stream );//m_xine->clock->get_current_time( m_xine->clock );
    audio_buffer_t *best_buf = 0;
    uint x = 0;


    /* MY GOD! Why do I have to do this!?
     * You see if I don't the metronom doesn't make the timestamps accurate! */
    memcpy( myMetronom, m_stream->metronom, sizeof( metronom_t ) );


    for( MyNode *prev = myList, *node = myList->next; node != myList; node = node->next )
    {
        audio_buffer_t *buf = &node->buf;

        if( buf->stream != 0 )
        {
            //debug() << "timestamp| pts:" << buf->vpts << " n:" << buf->num_frames << endl;

            buf->vpts = myMetronom->got_audio_samples( myMetronom, buf->vpts, buf->num_frames );
            buf->stream = 0;

            Log::buffSize += buf->num_frames;
            Log::buffCount++;
        }

        if( buf->vpts < current_vpts  )
        {
            if( prev != myList ) //thread-safety
            {
                //debug() << "dispose\n";

                prev->next = node->next;

                free( buf->mem );
                free( node );

                node = prev;
            }
        }
        else if( !best_buf || buf->vpts < best_buf->vpts ) best_buf = buf;

        ++Log::listSize;
        ++x;

        prev = node;
    }

    if( best_buf )
    {
        int64_t
        diff  = best_buf->vpts - current_vpts;
        diff *= myMetronom->audio_samples;
        diff /= myMetronom->pts_per_smpls;


        Log::diffSize += diff;
        Log::scopeRequests++;

        //debug() << "vpts:" << best_buf->vpts << " d:" << diff << " n:" << best_buf->num_frames << endl;

        if( diff < best_buf->num_frames - 512 ) //done this way as diff is a 64bit int => less cycles
        {
            const int16_t *data16 = best_buf->mem;
            data16 += diff * myChannels;

            for( int c, i = 0; i < 512; ++i, data16 += myChannels )
                for( m_scope[i] = 0, c = 0; c < myChannels; ++c )
                    m_scope[i] += data16[c];
        }
        else { Log::buffTooSmall++; }
    }
    else { Log::buffNotFound++; }

    return m_scope;
}

void
XineEngine::timerEvent( QTimerEvent* )
{
    //if scope() isn't called regularly the audio buffer list
    //is never emptied. This is a hacky solution, the better solution
    //is to prune the list inside the post_plugin put_buffer() function
    //which I will do eventually

    scope();
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
XineEngine::canDecode( const KURL &url )
{
    static QStringList list = QStringList::split( ' ', xine_get_file_extensions( m_xine ) );

    //TODO proper mimetype checking

    const QString path = url.path();
    const QString ext  = path.mid( path.findRev( '.' ) + 1 );
    return ext != "txt" && list.contains( ext );
}

void
XineEngine::customEvent( QCustomEvent *e )
{
    switch( e->type() )
    {
    case 3000:
        emptyMyList();
        emit trackEnded();
        break;

    case 3001:
        #define message static_cast<QString*>(e->data())
        KMessageBox::error( 0, (*message).arg( m_url.prettyURL() ) );
        delete message;
        #undef message
        break;

    case 3002:
        #define message static_cast<QString*>(e->data())
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
        msg = msg.arg( QString::fromUtf8( pd->description ) )
                 .arg( KGlobal::locale()->formatNumber( pd->percent, 0 ) );

        QApplication::postEvent( xe, new QCustomEvent(QEvent::Type(3002), new QString(msg)) );

        break;
    }
    case XINE_EVENT_UI_MESSAGE:
    {
        debug() << "xine message received\n";

        xine_ui_message_data_t *data = (xine_ui_message_data_t *)xineEvent->data;
        QString message;

        /* some code taken from the m_xine-ui - Copyright (C) 2000-2003 the m_xine project */
        switch( data->type )
        {
        case XINE_MSG_NO_ERROR:
        {
            /* copy strings, and replace '\0' separators by '\n' */
            char  c[2000];
            char *s = data->messages;
            char *d = c;

            while(s && (*s != '\0') && ((*s + 1) != '\0'))
            {
                switch(*s)
                {
                case '\0':
                    *d = '\n';
                    break;

                default:
                    *d = *s;
                    break;
                }

                s++;
                d++;
            }
            *++d = '\0';

            debug() << c << endl;

            break;
        }

        case XINE_MSG_ENCRYPTED_SOURCE: break;

        case XINE_MSG_UNKNOWN_HOST: message = i18n("The host is unknown for the url: <i>%1</i>"); goto param;
        case XINE_MSG_UNKNOWN_DEVICE: message = i18n("The device name you specified seems invalid."); goto param;
        case XINE_MSG_NETWORK_UNREACHABLE: message = i18n("The network appears unreachable."); goto param;
        case XINE_MSG_AUDIO_OUT_UNAVAILABLE: message = i18n("Audio output unavailable; the device is busy."); goto param;
        case XINE_MSG_CONNECTION_REFUSED: message = i18n("The connection was refused for the url: <i>%1</i>"); goto param;
        case XINE_MSG_FILE_NOT_FOUND: message = i18n("xine could not find the url: <i>%1</i>"); goto param;
        case XINE_MSG_PERMISSION_ERROR: message = i18n("Access was denied for the url: <i>%1</i>"); goto param;
        case XINE_MSG_READ_ERROR: message = i18n("The source cannot be read for the url: <i>%1</i>"); goto param;
        case XINE_MSG_LIBRARY_LOAD_ERROR: message = i18n("A problem occured while loading a library or decoder."); goto param;

        case XINE_MSG_GENERAL_WARNING: message = i18n("General Warning"); goto explain;
        case XINE_MSG_SECURITY:        message = i18n("Security Warning"); goto explain;
        default:                       message = i18n("Unknown Error"); goto explain;


        explain:

            if(data->explanation)
            {
                message.prepend( "<b>" );
                message += "</b>:<p>";
                message += ((char *) data + data->explanation);
            }
            else break; //if no explanation then why bother!


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
