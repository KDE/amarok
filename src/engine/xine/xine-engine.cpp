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
#include "plugin/plugin.h"

AMAROK_EXPORT_PLUGIN( XineEngine )

#include <kapplication.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <qdir.h>
#include <xine/xineutils.h>

#define this this_
#define XINE_ENGINE_INTERNAL
#define METRONOM_INTERNAL
    #include <xine/xine_internal.h> //for port_ticket from struct xine_t
    #include <xine/post.h>
#undef this


static inline kdbgstream
debug()
{
    return kdbgstream( "[xine-engine] ", 0, 0 );
}


XineEngine::XineEngine()
  : EngineBase()
  , m_xine( 0 )
  , m_stream( 0 )
  , m_audioPort( 0 )
  , m_eventQueue( 0 )
  , m_post( 0 )
{}

XineEngine::~XineEngine()
{
    //if (m_post)       xine_post_dispose(m_xine, m_post);
    if (m_stream)     xine_close(m_stream);
    if (m_eventQueue) xine_event_dispose_queue(m_eventQueue);
    if (m_stream)     xine_dispose(m_stream);
    if (m_audioPort)  xine_close_audio_driver(m_xine, m_audioPort);
    if (m_xine)       xine_exit(m_xine);

    debug() << "xine closed\n";
}

void
XineEngine::init( bool&, int, bool )
{
    debug() << "Enjoy the xine-engine. Please report bugs to amarok-devel@lists.sourceforge.net\n";

    m_xine = xine_new();

    if (!m_xine)
    {
        KMessageBox::error( 0, i18n("amaroK could not initialise xine.") );
        return;
    }

    //xine_engine_set_param( m_xine, XINE_ENGINE_PARAM_VERBOSITY, 99 );


    QString
    path  = QDir::homeDirPath();
    path += "/.%1/config";
    path  = QFile::exists( path.arg( "kaffeine" ) ) ? path.arg( "kaffeine" ) : path.arg( "xine" );

    xine_config_load( m_xine, path.local8Bit() );

    xine_init( m_xine );

    m_audioPort = xine_open_audio_driver( m_xine, "auto", NULL );
    if( !m_audioPort )
    {
        KMessageBox::error( 0, i18n("xine was unable to initialize any audio-drivers.") );
        return;
    }

    m_stream  = xine_stream_new( m_xine, m_audioPort, 0 );
    if( !m_stream )
    {
        KMessageBox::error( 0, i18n("amaroK could not create a new xine-stream.") );
        return;
    }

    //less buffering, faster seeking.. TODO test
    xine_set_param( m_stream, XINE_PARAM_METRONOM_PREBUFFER, 6000 );

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
}

void
XineEngine::play()
{
    if( xine_get_status( m_stream ) == XINE_STATUS_PLAY && !xine_get_param( m_stream, XINE_PARAM_SPEED ) )
    {
        xine_set_param(m_stream, XINE_PARAM_SPEED, XINE_SPEED_NORMAL);
        return;
    }

    xine_close( m_stream );

    if( xine_open( m_stream, m_url.url().local8Bit() ) )
    {
        if( xine_play( m_stream, 0, 0 ) )
        {
            xine_post_out_t *source = xine_get_audio_source( m_stream );
            xine_post_in_t  *target = (xine_post_in_t*)xine_post_input( m_post, const_cast<char*>("audio in") );

            xine_post_wire( source, target );

            return;
        }
    }

    //we should get a ui message from the event listener
    //KMessageBox::sorry( 0, i18n( "<p>m_xine could not open the media at: <i>%1</i>" ).arg( m_url.prettyURL() ) );
    emit stopped();
}

void
XineEngine::stop()
{
    xine_stop( m_stream );
    emit stopped();
}

void
XineEngine::pause()
{
    xine_set_param(m_stream, XINE_PARAM_SPEED, XINE_SPEED_PAUSE);
}

EngineBase::EngineState
XineEngine::state() const
{
    switch( xine_get_status( m_stream ) )
    {
    case XINE_STATUS_PLAY: return xine_get_param(m_stream, XINE_PARAM_SPEED) ? EngineBase::Playing : EngineBase::Paused;
    case XINE_STATUS_IDLE: return EngineBase::Idle;
    case XINE_STATUS_STOP:
    default:               return EngineBase::Empty;
    }
}

std::vector<float>*
XineEngine::scope()
{
    extern xine_list_t *myList;
    extern int myChannels;
    extern metronom_t *myMetronom;

    std::vector<float> &v = *(new std::vector<float>( 512 ));
    int64_t current_vpts = m_xine->clock->get_current_time( m_xine->clock );//m_stream->metronom->audio_vpts;
    uint x = 0;

    audio_buffer_t *best_buf = 0;
    audio_buffer_t *buf      = (audio_buffer_t*)xine_list_first_content( myList );


    while( buf )
    {
        if( buf->stream != 0 )
        {
            buf->vpts = myMetronom->got_audio_samples( myMetronom, buf->vpts, buf->num_frames );
            buf->stream = 0;
        }

        if( buf->vpts < current_vpts )
        {
            free( buf->mem );
            free( buf );
            xine_list_delete_current( myList );

            if( xine_list_is_empty( myList ) ) break;

            //xinelibs are gay, provide no get current list item function
            buf = (audio_buffer_t*)xine_list_prev_content( myList );
        }
        else if( !best_buf || buf->vpts < best_buf->vpts ) best_buf = buf;

        ++x;

        buf = (audio_buffer_t*)xine_list_next_content( myList );
    }

    if( best_buf )
    {
        uint
        diff  = best_buf->vpts - current_vpts;
        diff *= myMetronom->audio_samples;
        diff /= myMetronom->pts_per_smpls;

        //debug() << "chosen: " << best_buf->vpts << "| diff: " << best_buf->vpts - current_vpts << " buffer_offset: " << diff << "| list size: " << x << endl;


        if( diff+512 > best_buf->num_frames ) { debug() << "Not enough frames in this buffer!\n"; return &v; }


        const int16_t *data16 = best_buf->mem;
        data16 += diff*2;

        //TODO we assume there are enough buffers. There may not be...

        for( int a, c, i = 0; i < 512; ++i, data16 += myChannels )
        {
            for( a = 0, c = 0; c < myChannels; ++c ) a += data16[c];

            v[i] = (double)a / (1<<15);
        }
    }
    else debug() << "No best_buf found!\n";

    return &v;
}

long
XineEngine::position() const
{
    int pos;
    int time = 0;
    int length;

    xine_get_pos_length( m_stream, &pos, &time, &length );

    return time;
}

void
XineEngine::seek( long ms )
{
    xine_play( m_stream, 0, ms );
}

bool
XineEngine::initMixer( bool hardware )
{
    //ensure that software mixer volume is back to normal
    xine_set_param( m_stream, XINE_PARAM_AUDIO_AMP_LEVEL, 100 );

    m_mixerHW = hardware ? 0 : -1;
    return hardware;
}

void
XineEngine::setVolume( int vol )
{
    xine_set_param( m_stream, isMixerHardware() ? XINE_PARAM_AUDIO_VOLUME : XINE_PARAM_AUDIO_AMP_LEVEL, vol );
    m_volume = vol;
}

bool
XineEngine::canDecode( const KURL &url, mode_t, mode_t )
{
    //TODO proper mimetype checking

    const QString path = url.path();
    const QString ext  = path.mid( path.findRev( '.' ) + 1 );
    return QStringList::split( ' ', xine_get_file_extensions( m_xine ) ).contains( ext ) && ext != "txt";
}

void
XineEngine::customEvent( QCustomEvent *e )
{
    switch( e->type() )
    {
    case 3000:
        emit endOfTrack();
        break;

    case 3001:
        #define message static_cast<QString*>(e->data())
        KMessageBox::error( 0, (*message).arg( m_url.prettyURL() ) );
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
            else break; ///if no explanation then why bother!


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
