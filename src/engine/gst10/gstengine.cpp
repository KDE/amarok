/***************************************************************************
 *   Copyright (C) 2003-2005 by Mark Kretschmann <markey@web.de>           *
 *   Copyright (C) 2005 by Jakub Stachowski <qbast@go2.pl>                 *
 *   Portions Copyright (C) 2006 Paul Cifarelli <paul@cifarelli.net>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#define DEBUG_PREFIX "Gst-Engine"

#include "debug.h"

#include "config/gstconfig.h"
#include "equalizer/gstequalizer.h"
#include "enginebase.h"
#include "gstengine.h"
//#include "streamsrc.h"

#include <math.h>
#include <unistd.h>
#include <vector>

#include <qfile.h>
#include <qtimer.h>

#include <kapplication.h>
#include <kio/job.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kurl.h>

#include <gst/gst.h>

#define RETURN_IF_PIPELINE_EMPTY if ( !m_pipelineFilled ) return;


using std::vector;

GstEngine* GstEngine::s_instance;


AMAROK_EXPORT_PLUGIN( GstEngine )


/////////////////////////////////////////////////////////////////////////////////////
// CALLBACKS
/////////////////////////////////////////////////////////////////////////////////////

GstBusSyncReply
GstEngine::bus_cb(GstBus*, GstMessage* msg, gpointer) // static
{
   DEBUG_FUNC_INFO
   switch ( GST_MESSAGE_TYPE(msg))
   {
   	case GST_MESSAGE_ERROR:
	{
	    GError* error;
	    gchar* debug;
            gst_message_parse_error(msg,&error,&debug);
            instance()->m_gst_error = QString::fromAscii( error->message );
    	    instance()->m_gst_debug = QString::fromAscii( debug );
            QTimer::singleShot( 0, instance(), SLOT( handlePipelineError() ) );
	    break;
	}
	case GST_MESSAGE_EOS:
	     QTimer::singleShot( 0, instance(), SLOT( endOfStreamReached() ) );
	     break;
	case GST_MESSAGE_TAG:
	{
	     gchar* string=NULL;
    	     Engine::SimpleMetaBundle bundle;
	     GstTagList* taglist;
	     gst_message_parse_tag(msg,&taglist);
             bool success = false;

             if ( gst_tag_list_get_string( taglist, GST_TAG_TITLE, &string ) && string ) {
                  debug() << "received tag 'Title': " << QString( string ) << endl;
                  bundle.title = QString( string );
                  success = true;
             }
             if ( gst_tag_list_get_string( taglist, GST_TAG_ARTIST, &string ) && string ) {
                  debug() << "received tag 'Artist': " << QString( string ) << endl;
                  bundle.artist = QString( string );
                  success = true;
             }
             if ( gst_tag_list_get_string( taglist, GST_TAG_COMMENT, &string ) && string ) {
                  debug() << "received tag 'Comment': " << QString( string ) << endl;
                  bundle.comment = QString( string );
                  success = true;
             }
             if ( gst_tag_list_get_string( taglist, GST_TAG_ALBUM, &string ) && string ) {
                  debug() << "received tag 'Album': " << QString( string ) << endl;
                  bundle.album = QString( string );
                  success = true;
             }
             g_free( string );
             gst_tag_list_free(taglist);
             if ( success ) {
                  instance()->m_metaBundle = bundle;
                  QTimer::singleShot( 0, instance(), SLOT( newMetaData() ) );
             }
	     break;
	 }
	 default: ;
      }
      gst_message_unref( msg );
      return GST_BUS_DROP;
}


void
GstEngine::newPad_cb( GstElement*, GstPad* pad, gboolean, gpointer ) //static
{
    DEBUG_BLOCK

    GstPad* const audiopad = gst_element_get_pad( instance()->m_gst_audiobin, "sink" );

    if ( GST_PAD_IS_LINKED( audiopad ) ) {
        debug() << "audiopad is already linked. Unlinking old pad." << endl;
        gst_pad_unlink( audiopad, GST_PAD_PEER( audiopad ) );
    }

    gst_pad_link( pad, audiopad );

    gst_object_unref( audiopad );
}


void
GstEngine::handoff_cb( GstPad*, GstBuffer* buf, gpointer arg) //static
{
   GstEngine *thisObj = static_cast<GstEngine *>( arg );

   // push copy of the buffer onto the delay queue
   // we make a copy because we don't want volume or equalization to affect the scope
   // maybe data probes are already copies?  I don't know - can't find it in the documentation
   // so we'll make a copy for now and investigate further later...
   gpointer newbuf = gst_buffer_copy(buf);
   g_queue_push_tail(thisObj->m_delayq, newbuf);
}


void
GstEngine::candecode_newPad_cb( GstElement*, GstPad* pad, gboolean, gpointer ) //static
{
    DEBUG_FUNC_INFO
    GstCaps* caps = gst_pad_get_caps( pad );
    if (gst_caps_get_size(caps)>0) {
        GstStructure* str = gst_caps_get_structure( caps,0 );
        if (g_strrstr(gst_structure_get_name( str ), "audio" )) instance()->m_canDecodeSuccess = true;
    }
    gst_caps_unref( caps );
}

void
GstEngine::candecode_last_cb( GstElement*, gpointer ) //static
{
    DEBUG_FUNC_INFO
    instance()->m_canDecodeLast = true;
}
/*
void
GstEngine::kio_resume_cb() //static
{
    if ( instance()->m_transferJob && instance()->m_transferJob->isSuspended() ) {
        instance()->m_transferJob->resume();
        debug() << "RESUMING kio transfer.\n";
    }
}
*/

/////////////////////////////////////////////////////////////////////////////////////
// CLASS GSTENGINE
/////////////////////////////////////////////////////////////////////////////////////

GstEngine::GstEngine()
        : Engine::Base()
/*      , m_streamBuf( new char[STREAMBUF_SIZE] )
        , m_streamBuffering( false )
        , m_transferJob( 0 )
*/      , m_delayq(0)
        , m_current(0)
        , m_pipelineFilled( false )
        , m_fadeValue( 0.0 )
        , m_equalizerEnabled( false )
        , m_shutdown( false )
{
    DEBUG_FUNC_INFO

    addPluginProperty( "HasConfigure",  "true" );
    addPluginProperty( "HasEqualizer",  "true" );
    addPluginProperty( "HasKIO",        "false" );

    // initialize the scope delay queue
    m_delayq = g_queue_new();
}


GstEngine::~GstEngine()
{
    DEBUG_BLOCK

    destroyPipeline();

//    delete[] m_streamBuf;

    // Destroy scope delay queue
    g_queue_free(m_delayq);

    // Save configuration
    GstConfig::writeConfig();
    gst_deinit();
}


/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
/////////////////////////////////////////////////////////////////////////////////////

bool
GstEngine::init()
{
    DEBUG_BLOCK

    s_instance = this;

    // GStreamer initilization
    GError *err;
    if ( !gst_init_check( NULL, NULL, &err ) ) {
        KMessageBox::error( 0,
            i18n( "<h3>GStreamer could not be initialized.</h3> "
                  "<p>Please make sure that you have installed all necessary GStreamer plugins (e.g. OGG and MP3), and run <i>'gst-register'</i> afterwards.</p>"
                  "<p>For further assistance consult the GStreamer manual, and join #gstreamer on irc.freenode.net.</p>" ) );
        return false;
    }

    // Check if registry exists
    GstElement* dummy = gst_element_factory_make ( "fakesink", "fakesink" );
    if ( !dummy ) {
        KMessageBox::error( 0,
            i18n( "<h3>GStreamer is missing a registry.</h3> "
                  "<p>Please make sure that you have installed all necessary GStreamer plugins (e.g. OGG and MP3), and run <i>'gst-register'</i> afterwards.</p>"
                  "<p>For further assistance consult the GStreamer manual, and join #gstreamer on irc.freenode.net.</p>" ) );
        return false;
    }
    
    gst_object_unref( dummy );

    return true;
}


bool
GstEngine::canDecode( const KURL &url ) const
{

    // We had some bug reports claiming that video files cause crashes in canDecode(),
    // so don't try to decode them
    if ( url.fileName().lower().endsWith( ".mov" ) ||
         url.fileName().lower().endsWith( ".avi" ) ||
         url.fileName().lower().endsWith( ".wmv" ) )
        return false;

    debug() << "Can decode for " << url.prettyURL() << endl;
    int count = 0;
    m_canDecodeSuccess = false;
    m_canDecodeLast = false;
    GstElement *pipeline, *filesrc, *decodebin;

    if ( !( pipeline = createElement( "pipeline" ) ) ) return false;
    if ( !( filesrc = createElement( "filesrc", pipeline ) ) ) return false;
    if ( !( decodebin = createElement( "decodebin", pipeline ) ) ) return false;


    gst_element_link( filesrc, decodebin );

    g_object_set( G_OBJECT( filesrc ), "location", (const char*) QFile::encodeName( url.path() ), NULL );
    g_signal_connect( G_OBJECT( decodebin ), "new-decoded-pad", G_CALLBACK( candecode_newPad_cb ), NULL );
    g_signal_connect( G_OBJECT( decodebin ), "no-more-pads", G_CALLBACK( candecode_last_cb ), NULL );

    gst_element_set_state( pipeline, GST_STATE_PLAYING );

    // Wait until found audio stream

    while ( !m_canDecodeSuccess && !m_canDecodeLast && count < 100 ) {
        count++;
	usleep(1000);
    }

    debug() << "Got " << m_canDecodeSuccess << " after " << count << " sleeps" << endl;
    gst_element_set_state( pipeline, GST_STATE_NULL );
    gst_object_unref( GST_OBJECT( pipeline ) );

    return m_canDecodeSuccess;
}


uint
GstEngine::position() const
{
    if ( !m_pipelineFilled ) return 0;

    GstFormat fmt = GST_FORMAT_TIME;
    // Value will hold the current time position in nanoseconds. Must be initialized!
    gint64 value = 0;
    gst_element_query_position( m_gst_pipeline, &fmt, &value );

    return static_cast<uint>( ( value / GST_MSECOND ) ); // nanosec -> msec
}


uint
GstEngine::length() const
{
    if ( !m_pipelineFilled ) return 0;

    GstFormat fmt = GST_FORMAT_TIME;
    // Value will hold the track length in nanoseconds. Must be initialized!
    gint64 value = 0;
    gst_element_query_duration( m_gst_pipeline,  &fmt, &value );

    return static_cast<uint>( ( value / GST_MSECOND ) ); // nanosec -> msec
}


Engine::State
GstEngine::state() const
{
    if ( !m_pipelineFilled )
        return m_url.isEmpty() ? Engine::Empty : Engine::Idle;

    GstState s;
    gst_element_get_state( m_gst_pipeline, &s, NULL, GST_CLOCK_TIME_NONE );
    switch ( s )
    {
        case GST_STATE_NULL:
            return Engine::Empty;
        case GST_STATE_READY:
            return Engine::Idle;
        case GST_STATE_PLAYING:
            return Engine::Playing;
        case GST_STATE_PAUSED:
            return Engine::Paused;

        default:
            return Engine::Empty;
    }
}


const Engine::Scope&
GstEngine::scope()
{
   const int channels = 2;

   // prune the scope and get the current pos of the audio device
   gint64 pos = pruneScope();
   guint64 stime = 0, etime = 0, dur = 0;
   gint off, frames, sz;
   gint16 *data;

   // head of the delay queue is the most delayed, so we work with that one
   GstBuffer *buf = reinterpret_cast<GstBuffer *>( g_queue_peek_head(m_delayq) );
   if (buf)
   {
      // start time for this buffer
      stime = static_cast<guint64>( GST_BUFFER_TIMESTAMP( buf ) );
      // duration of the buffer...
      dur = static_cast<guint64>( GST_BUFFER_DURATION( buf ) );
      // therefore we can calculate the end time for the buffer
      etime = stime + dur;

      // if the audio device is playing this buffer now
      if (static_cast<guint64>(pos) > stime && static_cast<guint64>(pos) < etime)
      {
         // calculate the number of samples in the buffer
         sz = GST_BUFFER_SIZE(buf) / sizeof(guint16);
         // number of frames is the number of samples in each channel (frames like in the alsa sense)
         frames = sz / channels;

         // find the offset into the buffer to the sample closest to where the audio device is playing
         // it is the (time into the buffer cooresponding to the audio device pos) / (the sample rate)
         // sample rate = duration of the buffer / number of frames in the buffer
         // then we multiply by the number of channels to find the offset of the left channel sample 
         // of the frame in the buffer
         off = channels * (pos - stime) / (dur / frames);

         // note that we are assuming 16 bit samples, but this should probably be generalized...
         data = reinterpret_cast<gint16 *>( GST_BUFFER_DATA(buf) );
         if (off < sz) // better be...
         {
            int i = off; // starting at offset

            // loop while we fill the current buffer.  If we need another buffer and one is available, 
            // get it and keep filling.  If there are no more buffers available (not too likely)
            // then leave everything in this state and wait until the next time the scope updates
            while (buf && m_current < 512 && i < sz)
            {
               // convert to mono, for now - not generalize for >2 channels 'cause it'll be removed soon
               m_currentScope[m_current] = (data[i] + data[i+1]) / channels; 
               m_current++;
               i+=channels; // advance to the next frame
               if (i >= sz) // here we are out of samples in the current buffer, so we get another one
               {
                  buf = reinterpret_cast<GstBuffer *>( g_queue_pop_head(m_delayq) );
                  gst_buffer_unref(buf);
                  buf = reinterpret_cast<GstBuffer *>( g_queue_peek_head(m_delayq) );
                  if (buf)
                  {
                     stime = static_cast<guint64>( GST_BUFFER_TIMESTAMP( buf ) );
                     dur = static_cast<guint64>( GST_BUFFER_DURATION( buf ) );
                     etime = stime + dur;
                     i = 0;
                     sz = GST_BUFFER_SIZE(buf) / sizeof(guint16);
                     data = reinterpret_cast<gint16 *>( GST_BUFFER_DATA(buf) );
                  }
               }
            }
         }
      }
   }

   if (m_current >= 512)
   {
      // ok, we have a full buffer now, so give it to the scope
      for (int i=0; i< 512; i++)
         m_scope[i] = m_currentScope[i];
      m_current = 0;
   }

   return m_scope;

}


amaroK::PluginConfig*
GstEngine::configure() const
{
    DEBUG_FUNC_INFO

    GstConfigDialog* dialog = new GstConfigDialog( this );
    connect( dialog, SIGNAL( settingsSaved() ), SLOT( stop() ) );

    return dialog;
}

/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC SLOTS
/////////////////////////////////////////////////////////////////////////////////////

bool
GstEngine::load( const KURL& url, bool stream )  //SLOT
{
    DEBUG_BLOCK

    Engine::Base::load( url, stream );
    debug() << "Loading url: " << url.url() << endl;

    if ( !createPipeline() )
        return false;

    if ( url.isLocalFile() ) {
        // Use gst's filesrc element for local files, cause it's less overhead than KIO
        if ( !( m_gst_src = createElement( "filesrc", m_gst_pipeline ) ) ) { destroyPipeline(); return false; }
        // Set file path
        g_object_set( G_OBJECT(m_gst_src), "location", static_cast<const char*>( QFile::encodeName( url.path() ) ), NULL );
    }
      else { destroyPipeline(); return false; }
/*    else {
        // Create our custom streamsrc element, which transports data into the pipeline
        m_gst_src = GST_ELEMENT( gst_streamsrc_new( m_streamBuf, &m_streamBufIndex, &m_streamBufStop, &m_streamBuffering ) );
        gst_element_set( m_gst_src, "buffer_min", STREAMBUF_MIN, NULL );
        gst_bin_add ( GST_BIN ( m_gst_thread ), m_gst_src );
        g_signal_connect( G_OBJECT( m_gst_src ), "kio_resume", G_CALLBACK( kio_resume_cb ), m_gst_thread );

        m_streamBufIndex = 0;
        m_streamBufStop = false;
        m_streamBuffering = true;

        if ( !stream ) {
            // Use KIO for non-local files, except http, which is handled by StreamProvider
            m_transferJob = KIO::get( url, false, false );
            connect( m_transferJob, SIGNAL( data( KIO::Job*, const QByteArray& ) ), SLOT( newKioData( KIO::Job*, const QByteArray& ) ) );
            connect( m_transferJob, SIGNAL( result( KIO::Job* ) ), SLOT( kioFinished() ) );
        }
    }
*/
    if ( !( m_gst_decodebin = createElement( "decodebin", m_gst_pipeline ) ) ) { destroyPipeline(); return false; }
    g_signal_connect( G_OBJECT( m_gst_decodebin ), "new-decoded-pad", G_CALLBACK( newPad_cb ), NULL );

    // Link elements. The link from decodebin to audioconvert will be made in the newPad-callback
    gst_element_link( m_gst_src, m_gst_decodebin );

    setVolume( m_volume );
    setEqualizerEnabled( m_equalizerEnabled );
    if ( m_equalizerEnabled ) setEqualizerParameters( m_equalizerPreamp, m_equalizerGains );
    return true;
}


bool
GstEngine::play( uint offset )  //SLOT
{
    DEBUG_BLOCK

    // Try to play input pipeline; if fails, destroy input bin
    if ( !gst_element_set_state( m_gst_pipeline, GST_STATE_PLAYING ) ) {
        warning() << "Could not set thread to PLAYING.\n";
        destroyPipeline();
        return false;
    }

    // If "Resume playback on start" is enabled, we must seek to the last position
    if ( offset ) seek( offset );

    m_current = 0;
    startTimer( TIMER_INTERVAL );
    emit stateChanged( Engine::Playing );
    return true;
}


void
GstEngine::stop()  //SLOT
{
    DEBUG_BLOCK

    m_url = KURL(); // To ensure we return Empty from state()

    if ( m_pipelineFilled )
    {
        // Is a fade running?
        if ( m_fadeValue == 0.0 ) {
            // Not fading --> start fade now
            m_fadeValue = 1.0;
        }
        else
            // Fading --> stop playback
            destroyPipeline();
    }

    emit stateChanged( Engine::Empty );
}


void
GstEngine::pause()  //SLOT
{
    DEBUG_BLOCK
    RETURN_IF_PIPELINE_EMPTY

    if ( GST_STATE( m_gst_pipeline ) == GST_STATE_PAUSED ) {
        gst_element_set_state( m_gst_pipeline, GST_STATE_PLAYING );
        emit stateChanged( Engine::Playing );
    }
    else {
        gst_element_set_state( m_gst_pipeline, GST_STATE_PAUSED );
        emit stateChanged( Engine::Paused );
    }
}


void
GstEngine::seek( uint ms )  //SLOT
{
    RETURN_IF_PIPELINE_EMPTY

    if (!gst_element_seek(m_gst_pipeline, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH, GST_SEEK_TYPE_SET, ms*GST_MSECOND,
       GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) kdDebug() << "Seek failed" << endl; 
    gst_element_get_state(m_gst_pipeline, NULL, NULL, 100*GST_MSECOND);
}

/*
void
GstEngine::newStreamData( char* buf, int size )  //SLOT
{
    if ( m_streamBufIndex + size >= STREAMBUF_SIZE ) {
        m_streamBufIndex = 0;
        debug() << "Stream buffer overflow!" << endl;
    }

    sendBufferStatus();

    // Copy data into stream buffer
    memcpy( m_streamBuf + m_streamBufIndex, buf, size );
    // Adjust index
    m_streamBufIndex += size;
}
*/

void
GstEngine::setEqualizerEnabled( bool enabled ) //SLOT
{
    m_equalizerEnabled = enabled;

    RETURN_IF_PIPELINE_EMPTY

    g_object_set( G_OBJECT( m_gst_equalizer ), "active", enabled, NULL );
}


void
GstEngine::setEqualizerParameters( int preamp, const QValueList<int>& bandGains ) //SLOT
{
    m_equalizerPreamp = preamp;
    m_equalizerGains = bandGains;

    RETURN_IF_PIPELINE_EMPTY

    // BEGIN Preamp
    g_object_set( G_OBJECT( m_gst_equalizer ) , "preamp", ( preamp + 100 ) / 2 , NULL );
    // END

    // BEGIN Gains
    vector<int> gainsTemp;
    gainsTemp.resize( bandGains.count() );
    for ( uint i = 0; i < bandGains.count(); i++ )
        gainsTemp[i] = ( *bandGains.at( i ) + 100 ) / 2;

    g_object_set( G_OBJECT( m_gst_equalizer ), "gain", &gainsTemp, NULL );
    // END
}


/////////////////////////////////////////////////////////////////////////////////////
// PROTECTED
/////////////////////////////////////////////////////////////////////////////////////

void
GstEngine::setVolumeSW( uint percent )
{
    RETURN_IF_PIPELINE_EMPTY

    double fade;
    if ( m_fadeValue > 0.0 )
        fade = 1.0 - log10( ( 1.0 - m_fadeValue ) * 9.0 + 1.0 );
    else
        fade = 1.0;

    g_object_set( G_OBJECT(m_gst_volume), "volume", (double) percent * fade * 0.01, NULL );
}


void GstEngine::timerEvent( QTimerEvent* )
{
    // keep the scope from building while we are not visible
    // this is why the timer must run as long as we are playing, and not just when
    // we are fading
    pruneScope();

    // *** Volume fading ***

    // Are we currently fading?
    if ( m_fadeValue > 0.0 )
    {
        m_fadeValue -= ( GstConfig::fadeoutDuration() ) ?  1.0 / GstConfig::fadeoutDuration() * TIMER_INTERVAL : 1.0;

        // Fade finished?
        if ( m_fadeValue <= 0.0 ) {
            // Fade transition has finished, stop playback
            debug() << "[Gst-Engine] Fade-out finished.\n";
            destroyPipeline();
            killTimers();
        }
        setVolume( volume() );
    }
}


/////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
/////////////////////////////////////////////////////////////////////////////////////

void
GstEngine::handlePipelineError()  //SLOT
{
    DEBUG_BLOCK

    QString text = "[GStreamer Error] ";
    text += m_gst_error;

    if ( !m_gst_debug.isEmpty() ) {
        text += " ** ";
        text += m_gst_debug;
    }

    m_gst_error = QString();
    emit statusText( text );
    error() << text << endl;

    destroyPipeline();
}


void
GstEngine::endOfStreamReached()  //SLOT
{
    DEBUG_BLOCK

    destroyPipeline();
    emit trackEnded();
}

/*
void
GstEngine::newKioData( KIO::Job*, const QByteArray& array )  //SLOT
{
    const int size = array.size();

    if ( m_streamBufIndex >= STREAMBUF_MAX ) {
        debug() << "SUSPENDING kio transfer.\n";
        if ( m_transferJob ) m_transferJob->suspend();
    }

    if ( m_streamBufIndex + size >= STREAMBUF_SIZE ) {
        m_streamBufIndex = 0;
        debug() << "Stream buffer overflow!" << endl;
    }

    sendBufferStatus();

    // Copy data into stream buffer
    memcpy( m_streamBuf + m_streamBufIndex, array.data(), size );
    // Adjust index
    m_streamBufIndex += size;
}
*/

void
GstEngine::newMetaData()  //SLOT
{
    emit metaData( m_metaBundle );
}

/*
void
GstEngine::kioFinished()  //SLOT
{
    DEBUG_FUNC_INFO

    // KIO::Job deletes itself when finished, so we need to zero the pointer
    m_transferJob = 0;

    // Tell streamsrc: This is the end, my friend
    m_streamBufStop = true;
}
*/

void
GstEngine::errorNoOutput() //SLOT
{
    KMessageBox::information( 0, i18n( "<p>Please select a GStreamer <u>output plugin</u> in the engine settings dialog.</p>" ) );

    // Show engine settings dialog
    showEngineConfigDialog();
}


/////////////////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
/////////////////////////////////////////////////////////////////////////////////////

GstElement*
GstEngine::createElement( const QCString& factoryName, GstElement* bin, const QCString& name ) //static
{
    GstElement* element = gst_element_factory_make( factoryName, name );

    if ( element ) {
        if ( bin ) gst_bin_add( GST_BIN( bin ), element );
    }
    else {
        KMessageBox::error( 0,
            i18n( "<h3>GStreamer could not create the element: <i>%1</i></h3> "
                  "<p>Please make sure that you have installed all necessary GStreamer plugins (e.g. OGG and MP3), and run <i>'gst-register'</i> afterwards.</p>"
                  "<p>For further assistance consult the GStreamer manual, and join #gstreamer on irc.freenode.net.</p>" ).arg( factoryName ) );
        gst_object_unref( GST_OBJECT( bin ) );
    }

    return element;
}


QStringList
GstEngine::getPluginList( const QCString& classname ) const
{
    GList* features = NULL;
    QString name;
    QStringList results;

    GstRegistry* registry = gst_registry_get_default();
    features = gst_registry_get_feature_list(registry,GST_TYPE_ELEMENT_FACTORY);
        while ( features ) {
        GstElementFactory * factory = GST_ELEMENT_FACTORY ( features->data );
        if ( g_strrstr ( factory->details.klass, classname ) ) {
            name = g_strdup ( GST_PLUGIN_FEATURE_NAME ( features->data ) );
            if ( name != "autoaudiosink" )
                results << name;
            }
        features = g_list_next ( features );
        }
    gst_plugin_feature_list_free(features);
    return results;
}


bool
GstEngine::createPipeline()
{
    DEBUG_BLOCK

    destroyPipeline();

    if ( GstConfig::soundOutput().isEmpty()) {
        QTimer::singleShot( 0, this, SLOT( errorNoOutput() ) );
        return false;
    }
    debug() << "Sound output method: " << GstConfig::soundOutput() << endl;
    debug() << "CustomSoundDevice: " << ( GstConfig::useCustomSoundDevice() ? "true" : "false" ) << endl;
    debug() << "Sound Device: " << GstConfig::soundDevice() << endl;
    debug() << "CustomOutputParams: " << ( GstConfig::useCustomOutputParams() ? "true" : "false" ) << endl;
    debug() << "Output Params: " << GstConfig::outputParams() << endl;

    // Let gst construct the output element from a string
    QCString output  = GstConfig::soundOutput().latin1();
    if ( GstConfig::useCustomOutputParams() ) {
        output += " ";
        output += GstConfig::outputParams().latin1();
    }

    m_gst_pipeline = gst_pipeline_new( "pipeline" );
    m_gst_audiobin = gst_bin_new( "audiobin" );


    if ( !( m_gst_audiosink = createElement( output, m_gst_audiobin ) ) ) {
        QTimer::singleShot( 0, this, SLOT( errorNoOutput() ) );
        return false;
    }

    /* setting device property for AudioSink*/
    if ( GstConfig::useCustomSoundDevice() && !GstConfig::soundDevice().isEmpty() )
        g_object_set( G_OBJECT(m_gst_audiosink), "device", GstConfig::soundDevice().latin1(), NULL );

    m_gst_equalizer = GST_ELEMENT( gst_equalizer_new() );
    gst_bin_add( GST_BIN( m_gst_audiobin ), m_gst_equalizer );
    if ( !( m_gst_audioconvert = createElement( "audioconvert", m_gst_audiobin ) ) ) { return false; }
    if ( !( m_gst_identity = createElement( "identity", m_gst_audiobin ) ) ) { return false; }
    if ( !( m_gst_volume = createElement( "volume", m_gst_audiobin ) ) ) { return false; }
    if ( !( m_gst_audioscale = createElement( "audioresample", m_gst_audiobin ) ) ) { return false; }

    GstPad* p;
    p = gst_element_get_pad(m_gst_audioconvert, "sink");
    gst_element_add_pad(m_gst_audiobin,gst_ghost_pad_new("sink",p));
    gst_object_unref(p);

    // add a data probe on the src pad if the audioconvert element for our scope
    // we do it here because we want pre-equalized and pre-volume samples
    // so that our visualization are not affected by them
    p = gst_element_get_pad (m_gst_audioconvert, "src");
    gst_pad_add_buffer_probe (p, G_CALLBACK(handoff_cb), this);
    gst_object_unref (p);

    /* link elements */
    gst_element_link_many( m_gst_audioconvert, m_gst_equalizer, m_gst_identity,
                           m_gst_volume, m_gst_audioscale, m_gst_audiosink, NULL );

    gst_bin_add( GST_BIN(m_gst_pipeline), m_gst_audiobin);
    gst_bus_set_sync_handler(gst_pipeline_get_bus(GST_PIPELINE(m_gst_pipeline)), bus_cb, NULL);

    m_pipelineFilled = true;
    return true;
}


void
GstEngine::destroyPipeline()
{
    DEBUG_BLOCK

    m_fadeValue = 0.0;

    clearScopeQ();

    if ( m_pipelineFilled ) {
        debug() << "Unreffing pipeline." << endl;
        gst_element_set_state( m_gst_pipeline, GST_STATE_NULL );
        gst_object_unref( GST_OBJECT( m_gst_pipeline ) );

        m_pipelineFilled = false;
    }

    // Destroy KIO transmission job
/*    if ( m_transferJob ) {
        m_transferJob->kill();
        m_transferJob = 0;
    } */
}

/*
void
GstEngine::sendBufferStatus()
{
    if ( m_streamBuffering ) {
        const int percent = (int) ( (float) m_streamBufIndex / STREAMBUF_MIN * 105.0 );
        emit statusText( i18n( "Buffering.. %1%" ).arg( MIN( percent, 100 ) ) );
    }
}
*/

gint64 GstEngine::pruneScope()
{
    if ( !m_pipelineFilled ) return 0; // dont prune if we aren't playing

    // get the position playing in the audio device
    GstFormat fmt = GST_FORMAT_TIME;
    gint64 pos = 0;
    gst_element_query_position( m_gst_pipeline, &fmt, &pos );

    GstBuffer *buf = 0;
    guint64 stime, etime, dur;
  
    // free up the buffers that the audio device has advanced past already 
    do
    {
       // most delayed buffers are at the head of the queue
       buf = reinterpret_cast<GstBuffer *>( g_queue_peek_head(m_delayq) );
       if (buf)
       {
          // the start time of the buffer
          stime = static_cast<guint64>( GST_BUFFER_TIMESTAMP( buf ) );
          // the duration of the buffer
          dur = static_cast<guint64>( GST_BUFFER_DURATION( buf ) );
          // therefore we can calculate the end time of the buffer
          etime = stime + dur;

          // purge this buffer if the pos is past the end time of the buffer
          if (static_cast<guint64>(pos) > etime)
          {
             g_queue_pop_head(m_delayq);
             gst_buffer_unref(buf);
          }
       }
    } while (buf && static_cast<guint64>(pos) > etime);

    return pos;
}

void GstEngine::clearScopeQ()
{
   GstBuffer *buf;

   // just free them all
   while (g_queue_get_length(m_delayq))
   {
      buf = reinterpret_cast<GstBuffer *>( g_queue_pop_head(m_delayq) );
      gst_buffer_unref(buf);
   }
}

#include "gstengine.moc"
