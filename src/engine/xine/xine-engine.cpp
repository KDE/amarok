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
    return kdbgstream( "[XineEngine] ", 0, 0 );
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

    debug() << "Xine closed\n";
}

void
XineEngine::init( bool&, int, bool )
{
    debug() << "Initialising Xine...\n";

    xineEngine = xine_new();

    if (!xineEngine)
    {
        KMessageBox::error( 0, i18n("amaroK could not initialise Xine.") );
        return;
    }

    xine_config_load( xineEngine, (QDir::homeDirPath() + "/.kaffeine/config").local8Bit() );
    xine_init( xineEngine );


    /** set xine parameters **/

    const char* const* drivers = NULL;
    char **audioChoices = new char*[15];
    int i = 0;

    drivers = xine_list_audio_output_plugins (xineEngine);

    audioChoices[0] = new char[10];
    audioChoices[0] = const_cast<char*>("auto");

    for( i = 0; drivers[i]; ++i )
    {
        audioChoices[i+1] = new char[10];
        strcpy( audioChoices[i+1], (char*)drivers[i] );
    }

    audioChoices[i+1] = NULL;

    char* audioInfo = new char[200];
    strcpy( audioInfo, i18n("Audiodriver to use (default: auto)").utf8() );
    i = xine_config_register_enum(xineEngine, "gui.audiodriver", 0, audioChoices, audioInfo, NULL, 10, &XineEngine::AudioDriverChangedCallback, this);

    audioDriverName = audioChoices[i];


    char* mixerInfo = new char[200];
    strcpy( mixerInfo, i18n("Use software audio mixer").utf8() );
    /*m_mixerHW = !(bool)*/xine_config_register_bool(xineEngine, "gui.audio_mixer_software", 1, mixerInfo, NULL, 10, &XineEngine::AudioMixerMethodChangedCallback, this);


    debug() << "Init audio driver: " << audioDriverName << endl;

    audioDriver = xine_open_audio_driver( xineEngine, audioDriverName.latin1(), NULL );

    if (!audioDriver && audioDriverName != "auto")
    {
        debug() << "Driver init failed. Trying 'auto'...\n";
        audioDriverName = "auto";
        audioDriver = xine_open_audio_driver (xineEngine, audioDriverName.latin1(), NULL);
    }

    if (!audioDriver)
    {
        KMessageBox::error(0, i18n("Xine was unable to initialize any audio-drivers."));
        return;
    }


    debug() << "Open stream\n";

    xineStream  = xine_stream_new( xineEngine, audioDriver, 0 );
    if (!xineStream)
    {
        KMessageBox::error( 0, i18n("amaroK could not create a new XineStream.") );
        return;
    }


    eventQueue = xine_event_new_queue(xineStream);
    xine_event_create_listener_thread( eventQueue, &XineEngine::XineEventListener, (void*)this );


    debug() << "Xine successfully initialised\n";

    return;
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

    KMessageBox::sorry( 0, i18n( "<p>Xine could not open the media at: <i>%1</i>" ).arg( m_url.prettyURL() ) );
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

void
XineEngine::setVolume( int vol )
{
    if( m_mixerHW )
    {
        xine_set_param(xineStream, XINE_PARAM_AUDIO_AMP_LEVEL, -vol*2);
    }
    else
    {
        xine_set_param(xineStream, XINE_PARAM_AUDIO_VOLUME, -vol);
    }
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
        KMessageBox::error( 0, *message );
        delete message;
        #undef message
        break;

    default:
        ;
    }
}


#define xe static_cast<XineEngine*>(p)

void
XineEngine::AudioDriverChangedCallback( void* p, xine_cfg_entry_t* entry )
{
    if( p == NULL ) return;

    debug() << "New audio driver: " << entry->enum_values[entry->num_value] << endl;

    xine_close(xe->xineStream);
    xine_event_dispose_queue(xe->eventQueue);
    xine_dispose(xe->xineStream);
    xine_close_audio_driver(xe->xineEngine, xe->audioDriver);

    xe->audioDriver = 0;
    xe->audioDriver = xine_open_audio_driver(xe->xineEngine, entry->enum_values[entry->num_value], NULL);

    if (!xe->audioDriver)
    {
        xe->audioDriver = xine_open_audio_driver(xe->xineEngine, xe->audioDriverName.latin1(), NULL);
    }
    else
    {
        xe->audioDriverName = entry->enum_values[entry->num_value];
    }

    xe->xineStream = xine_stream_new( xe->xineEngine, xe->audioDriver, NULL );
    xe->eventQueue = xine_event_new_queue (xe->xineStream);
    xine_event_create_listener_thread(xe->eventQueue, &XineEngine::XineEventListener, p);

    xe->play();
}

void
XineEngine::AudioMixerMethodChangedCallback(void* p, xine_cfg_entry_t* entry)
{
    if( p ) xe->m_mixerHW = (bool)entry->num_value;
}

void
XineEngine::XineEventListener( void *p, const xine_event_t* xineEvent )
{
    if( !p ) return;

    switch( xineEvent->type )
    {
    case XINE_EVENT_UI_PLAYBACK_FINISHED:

        //emit signal from GUI thread
        QApplication::postEvent( xe, new QCustomEvent(3000) );
        break;

    case XINE_EVENT_UI_MESSAGE:
    {
        debug() << "Xine event: Xine message\n";

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

        case XINE_MSG_UNKNOWN_HOST: message = i18n("The specified host is unknown."); goto param;
        case XINE_MSG_UNKNOWN_DEVICE: message = i18n("The device name you specified seems invalid."); goto param;
        case XINE_MSG_NETWORK_UNREACHABLE: message = i18n("The network appears unreachable."); goto param;
        case XINE_MSG_AUDIO_OUT_UNAVAILABLE: message = i18n("Audio output unavailable. The device is busy."); goto param;
        case XINE_MSG_CONNECTION_REFUSED: message = i18n("The connection was refused."); goto param;
        case XINE_MSG_FILE_NOT_FOUND: message = i18n("The specified file or url was not found."); goto param;
        case XINE_MSG_PERMISSION_ERROR: message = i18n("Permission to this source was denied."); goto param;
        case XINE_MSG_READ_ERROR: message = i18n("The source cannot be read."); goto param;
        case XINE_MSG_LIBRARY_LOAD_ERROR: message = i18n("A problem occured while loading a library or decoder."); goto param;

        case XINE_MSG_GENERAL_WARNING: message = i18n("General Warning"); goto explain;
        case XINE_MSG_SECURITY:        message = i18n("Security Warning"); goto explain;
        default:                       message = i18n("Unknown Error"); goto explain;


        explain:

            message.prepend( "<b>" );
            message += "</b>:\n";

            if(data->explanation)
            {
                message += ((char *) data + data->explanation);
                message += ' ';
            }


        param:

            if(data->explanation)
            {
                message += "<p>Xine Paramaters:\n<i>";
                message += ((char *) data + data->parameters);
                message += "</i>";
            }
            else message += i18n("No information available");

            message.prepend( "<p>" );

            QApplication::postEvent( xe, new QCustomEvent(QEvent::Type(3001), new QString(message)) );
        }

    } //case
    } //switch
}

#undef xe

#include "xine-engine.moc"
