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


static inline kdbgstream
debug()
{
    return kdbgstream( "[xine-engine] ", 0, 0 );
}


XineEngine::XineEngine()
  : EngineBase()
  , xineEngine( 0 )
  , xineStream( 0 )
  , audioDriver( 0 )
  , eventQueue( 0 )
{}

XineEngine::~XineEngine()
{
    if (xineStream)  xine_close(xineStream);
    if (eventQueue)  xine_event_dispose_queue(eventQueue);
    if (xineStream)  xine_dispose(xineStream);
    if (audioDriver) xine_close_audio_driver(xineEngine, audioDriver);
    if (xineEngine)  xine_exit(xineEngine);

    debug() << "xine closed\n";
}

void
XineEngine::init( bool&, int, bool )
{
    debug() << "Initialising xine...\n";

    xineEngine = xine_new();

    if (!xineEngine)
    {
        KMessageBox::error( 0, i18n("amaroK could not initialise xine.") );
        return;
    }

    QString
    path  = QDir::homeDirPath();
    path += "/.%1/config";
    path  = QFile::exists( path.arg( "kaffeine" ) ) ? path.arg( "kaffeine" ) : path.arg( "xine" );

    xine_config_load( xineEngine, path.local8Bit() );

    xine_init( xineEngine );

    audioDriver = xine_open_audio_driver( xineEngine, "auto", NULL );
    if( !audioDriver )
    {
        KMessageBox::error( 0, i18n("xine was unable to initialize any audio-drivers.") );
        return;
    }

    xineStream  = xine_stream_new( xineEngine, audioDriver, 0 );
    if( !xineStream )
    {
        KMessageBox::error( 0, i18n("amaroK could not create a new xine-stream.") );
        return;
    }

    eventQueue = xine_event_new_queue( xineStream );
    xine_event_create_listener_thread( eventQueue, &XineEngine::XineEventListener, (void*)this );
}

void
XineEngine::play()
{
    if( xine_get_status( xineStream ) == XINE_STATUS_PLAY && !xine_get_param( xineStream, XINE_PARAM_SPEED ) )
    {
        xine_set_param(xineStream, XINE_PARAM_SPEED, XINE_SPEED_NORMAL);
        return;
    }

    xine_close( xineStream );

    if( xine_open( xineStream, m_url.url().local8Bit() ) )
    {
        if( xine_play( xineStream, 0, 0 ) ) return;
    }

    //we should get a ui message from the event listener
    //KMessageBox::sorry( 0, i18n( "<p>xine could not open the media at: <i>%1</i>" ).arg( m_url.prettyURL() ) );
    emit stopped();
}

void
XineEngine::stop()
{
    /*if( xine_get_status(xineStream) == XINE_STATUS_PLAY ) */
    xine_stop( xineStream );
}

void
XineEngine::pause()
{
    xine_set_param(xineStream, XINE_PARAM_SPEED, XINE_SPEED_PAUSE);
}

EngineBase::EngineState
XineEngine::state() const
{
    switch( xine_get_status( xineStream ) )
    {
    case XINE_STATUS_PLAY: return xine_get_param(xineStream, XINE_PARAM_SPEED) ? EngineBase::Playing : EngineBase::Paused;
    case XINE_STATUS_IDLE: return EngineBase::Idle;
    case XINE_STATUS_STOP:
    default:               return EngineBase::Empty;
    }
}

long
XineEngine::position() const
{
    int pos;
    int time = 0;
    int length;

    xine_get_pos_length( xineStream, &pos, &time, &length );

    return time;
}

void
XineEngine::seek( long ms )
{
    if( xine_get_status( xineStream ) == XINE_STATUS_PLAY )
    {
        xine_play( xineStream, 0, ms );
    }
}

bool
XineEngine::initMixer( bool hardware )
{
    //ensure that software mixer volume is back to normal
    xine_set_param( xineStream, XINE_PARAM_AUDIO_AMP_LEVEL, 100 );

    m_mixerHW = hardware ? 0 : -1;
    return hardware;
}

void
XineEngine::setVolume( int vol )
{
    xine_set_param( xineStream, isMixerHardware() ? XINE_PARAM_AUDIO_VOLUME : XINE_PARAM_AUDIO_AMP_LEVEL, vol );
}

bool
XineEngine::canDecode( const KURL &url, mode_t, mode_t )
{
    //TODO proper mimetype checking

    const QString path = url.path();
    const QString ext  = path.mid( path.findRev( '.' ) + 1 );
    return QStringList::split( ' ', xine_get_file_extensions( xineEngine ) ).contains( ext ) && ext != "txt";
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

        /* some code taken from the xine-ui - Copyright (C) 2000-2003 the xine project */
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
            else message += i18n("No additional information is available.");

            QApplication::postEvent( xe, new QCustomEvent(QEvent::Type(3001), new QString(message)) );
        }

    } //case
    } //switch

    #undef xe
}

#include "xine-engine.moc"
