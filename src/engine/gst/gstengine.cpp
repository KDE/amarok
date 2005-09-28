/***************************************************************************
 *   Copyright (C) 2003-2005 by Mark Kretschmann <markey@web.de>           *
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
#include "streamsrc.h"

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

void
GstEngine::eos_cb( GstElement* /*element*/, gpointer ) //static
{
    DEBUG_FUNC_INFO

    //this is the Qt equivalent to an idle function: delay the call until all events are finished,
    //otherwise gst will crash horribly
    QTimer::singleShot( 0, instance(), SLOT( endOfStreamReached() ) );
}


void
GstEngine::newPad_cb( GstElement*, GstPad* pad, gboolean, gpointer ) //static
{
    DEBUG_BLOCK

    GstPad* const audiopad = gst_element_get_pad( instance()->m_gst_audioconvert, "sink" );

    if ( GST_PAD_IS_LINKED( audiopad ) ) {
        debug() << "audiopad is already linked. Unlinking old pad." << endl;
        gst_pad_unlink( audiopad, GST_PAD_PEER( audiopad ) );
    }

    gst_pad_link( pad, audiopad );

    if ( gst_element_get_parent( instance()->m_gst_audiobin ) == NULL )
        gst_bin_add( GST_BIN( instance()->m_gst_thread ), instance()->m_gst_audiobin );

    gst_bin_sync_children_state( GST_BIN( instance()->m_gst_thread ) );
}


void
GstEngine::handoff_cb( GstElement*, GstBuffer* buf, gpointer ) //static
{
    instance()->m_mutexScope.lock();

    // Check for buffer overflow
    const uint available = gst_adapter_available( instance()->m_gst_adapter );
    if ( available > SCOPEBUF_SIZE )
        gst_adapter_flush( instance()->m_gst_adapter, available - 30000 );

    // TODO On some systems buf is always 0. Why?
    if ( buf ) {
        gst_buffer_ref( buf );
        // Push buffer into adapter, where it's chopped into chunks
        gst_adapter_push( instance()->m_gst_adapter, buf );
    }

    instance()->m_mutexScope.unlock();
}


void
GstEngine::candecode_handoff_cb( GstElement*, GstBuffer*, gpointer ) //static
{
    DEBUG_FUNC_INFO

    instance()->m_canDecodeSuccess = true;
}


void
GstEngine::found_tag_cb( GstElement*, GstElement*, GstTagList* taglist, gpointer ) //static
{
    DEBUG_BLOCK

    char* string;
    Engine::SimpleMetaBundle bundle;
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

    if ( success ) {
        instance()->m_metaBundle = bundle;
        QTimer::singleShot( 0, instance(), SLOT( newMetaData() ) );
    }
}


void
GstEngine::pipelineError_cb( GstElement* /*element*/, GstElement* /*domain*/, GError* error, gchar* debug, gpointer /*data*/ ) //static
{
    DEBUG_FUNC_INFO

    instance()->m_gst_error = QString::fromAscii( error->message );
    instance()->m_gst_debug = QString::fromAscii( debug );

    // Process error message in application thread
    QTimer::singleShot( 0, instance(), SLOT( handlePipelineError() ) );
}


void
GstEngine::kio_resume_cb() //static
{
    if ( instance()->m_transferJob && instance()->m_transferJob->isSuspended() ) {
        instance()->m_transferJob->resume();
        debug() << "RESUMING kio transfer.\n";
    }
}


void
GstEngine::shutdown_cb() //static
{
    instance()->m_shutdown = true;
    debug() << "Thread is shut down.\n";
}


/////////////////////////////////////////////////////////////////////////////////////
// CLASS GSTENGINE
/////////////////////////////////////////////////////////////////////////////////////

GstEngine::GstEngine()
        : Engine::Base()
        , m_gst_adapter( 0 )
        , m_streamBuf( new char[STREAMBUF_SIZE] )
        , m_streamBuffering( false )
        , m_transferJob( 0 )
        , m_pipelineFilled( false )
        , m_fadeValue( 0.0 )
        , m_equalizerEnabled( false )
        , m_shutdown( false )
{
    DEBUG_FUNC_INFO

    addPluginProperty( "StreamingMode", "Signal" );
    addPluginProperty( "HasConfigure",  "true" );
    addPluginProperty( "HasEqualizer",  "true" );
    addPluginProperty( "HasKIO",        "true" );
}


GstEngine::~GstEngine()
{
    DEBUG_BLOCK
    debug() << "bytes left in gst_adapter: " << gst_adapter_available( m_gst_adapter ) << endl;

    if ( m_pipelineFilled ) {
        g_signal_connect( G_OBJECT( m_gst_thread ), "shutdown", G_CALLBACK( shutdown_cb ), m_gst_thread );
        destroyPipeline();
        // Wait for pipeline to shut down properly
        while ( !m_shutdown ) ::usleep( 20000 ); // 20 msec
    }
    else
        destroyPipeline();

    delete[] m_streamBuf;

    // Destroy scope adapter
    g_object_unref( G_OBJECT( m_gst_adapter ) );

    // Save configuration
    GstConfig::writeConfig();
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
    if ( !gst_init_check( NULL, NULL ) ) {
        KMessageBox::error( 0,
            i18n( "<h3>GStreamer could not be initialized.</h3> "
                  "<p>Please make sure that you have installed all necessary GStreamer plugins (e.g. OGG and MP3), and run <i>'gst-register'</i> afterwards.</p>"
                  "<p>For further assistance consult the GStreamer manual, and join #gstreamer on irc.freenode.net.</p>" ) );
        return false;
    }

    m_gst_adapter = gst_adapter_new();

    // Check if registry exists
    GstElement* dummy = gst_element_factory_make ( "fakesink", "fakesink" );
    if ( !dummy || !gst_scheduler_factory_make( NULL, GST_ELEMENT ( dummy ) ) ) {
        KMessageBox::error( 0,
            i18n( "<h3>GStreamer is missing a registry.</h3> "
                  "<p>Please make sure that you have installed all necessary GStreamer plugins (e.g. OGG and MP3), and run <i>'gst-register'</i> afterwards.</p>"
                  "<p>For further assistance consult the GStreamer manual, and join #gstreamer on irc.freenode.net.</p>" ) );
        return false;
    }

    return true;
}


bool
GstEngine::canDecode( const KURL &url ) const
{
    // TODO Consider using decodebin here as well

    // We had some bug reports claiming that video files cause crashes in canDecode(),
    // so don't try to decode them
    if ( url.fileName().lower().endsWith( ".mov" ) ||
         url.fileName().lower().endsWith( ".avi" ) ||
         url.fileName().lower().endsWith( ".wmv" ) )
        return false;

    int count = 0;
    m_canDecodeSuccess = false;
    GstElement *pipeline, *filesrc, *spider, *fakesink;

    if ( !( pipeline = createElement( "pipeline" ) ) ) return false;
    if ( !( filesrc = createElement( "filesrc", pipeline ) ) ) return false;
    if ( !( spider = createElement( "spider", pipeline ) ) ) return false;
    if ( !( fakesink = createElement( "fakesink", pipeline ) ) ) return false;

    GstCaps* filtercaps = gst_caps_new_simple( "audio/x-raw-int", NULL );

    gst_element_link( filesrc, spider );
    gst_element_link_filtered( spider, fakesink, filtercaps );

    gst_element_set( filesrc, "location", (const char*) QFile::encodeName( url.path() ), NULL );
    gst_element_set( fakesink, "signal_handoffs", true, NULL );
    g_signal_connect( G_OBJECT( fakesink ), "handoff", G_CALLBACK( candecode_handoff_cb ), pipeline );

    gst_element_set_state( pipeline, GST_STATE_PLAYING );

    // Try to iterate over the bin until signal "handoff" gets triggered
    while ( gst_bin_iterate( GST_BIN( pipeline ) ) && !m_canDecodeSuccess && count < 1000 )
        count++;

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
    gst_element_query( m_gst_decodebin, GST_QUERY_POSITION, &fmt, &value );

    return static_cast<uint>( ( value / GST_MSECOND ) ); // nanosec -> msec
}


uint
GstEngine::length() const
{
    DEBUG_BLOCK

    if ( !m_pipelineFilled ) return 0;

    GstFormat fmt = GST_FORMAT_TIME;
    // Value will hold the track length in nanoseconds. Must be initialized!
    gint64 value = 0;
    gst_element_query( m_gst_decodebin, GST_QUERY_TOTAL, &fmt, &value );

    return static_cast<uint>( ( value / GST_MSECOND ) ); // nanosec -> msec
}


Engine::State
GstEngine::state() const
{
    if ( !m_pipelineFilled )
        return m_url.isEmpty() ? Engine::Empty : Engine::Idle;

    switch ( gst_element_get_state( m_gst_thread ) )
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

    if ( gst_adapter_available( m_gst_adapter ) < SCOPE_VALUES*channels*sizeof( gint16 ) )
        return m_scope;

    m_mutexScope.lock();

    guint64 firstStamp, lastStamp;
    GstBuffer* buf;
    GSList* list = m_gst_adapter->buflist;

    // Get timestamp from first buffer
    buf = (GstBuffer*) g_slist_nth_data( list, 0 );
    firstStamp = GST_BUFFER_TIMESTAMP( buf );

    // Get timestamp from last buffer
    GSList* last = g_slist_last( list );
    buf = (GstBuffer*) last->data;
    lastStamp = GST_BUFFER_TIMESTAMP( buf );

    // Get current clock time from sink
    GstFormat fmt = GST_FORMAT_TIME;
    gint64 sinkStamp = 0; // Must be initalised to 0
    gst_element_query( m_gst_audiosink, GST_QUERY_POSITION, &fmt, &sinkStamp );

    guint available = gst_adapter_available( m_gst_adapter );
    gint16* data = (gint16*) gst_adapter_peek( m_gst_adapter, available );

    double factor = (double) ( lastStamp - sinkStamp ) / ( lastStamp - firstStamp );
    int offset = available - static_cast<int>( factor * (double) available );
    offset /= channels;
    offset *= channels;
    if ( offset < 0 ) offset = -offset; //FIXME Offset should never become < 0. Find out why this happens.
    offset = QMIN( (guint) offset, available - SCOPE_VALUES*channels*sizeof( gint16 ) );

    for ( long i = 0; i < SCOPE_VALUES; i++, data += channels ) {
        long temp = 0;

        for ( int chan = 0; chan < channels; chan++ ) {
            // Add all channels together so we effectively get a mono scope
            temp += data[offset / sizeof( gint16 ) + chan];
        }
        m_scope[i] = temp / channels;
    }

//     debug() << "Timestamp first: " << firstStamp << endl;
//     debug() << "Timestamp last:  " << lastStamp << endl;
//     debug() << "Timestamp sink:  " << sinkStamp << endl;
//     debug() << "factor: " << factor << endl;
//     debug() << "offset: " << offset << endl;
//     debug() << endl;

    m_mutexScope.unlock();
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

    m_gst_thread = gst_thread_new( "thread" );
    g_signal_connect( G_OBJECT( m_gst_thread ), "error", G_CALLBACK ( pipelineError_cb ), NULL );

    if ( url.isLocalFile() ) {
        // Use gst's filesrc element for local files, cause it's less overhead than KIO
        if ( !( m_gst_src = createElement( "filesrc", m_gst_thread ) ) ) { destroyPipeline(); return false; }
        // Set file path
        gst_element_set( m_gst_src, "location", static_cast<const char*>( QFile::encodeName( url.path() ) ), NULL );
    }
    else {
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

    if ( !( m_gst_decodebin = createElement( "decodebin", m_gst_thread ) ) ) { destroyPipeline(); return false; }
    g_signal_connect( G_OBJECT( m_gst_decodebin ), "new-decoded-pad", G_CALLBACK( newPad_cb ), NULL );
    g_signal_connect( G_OBJECT( m_gst_decodebin ), "found-tag", G_CALLBACK( found_tag_cb ), NULL );
    g_signal_connect( G_OBJECT( m_gst_decodebin ), "eos", G_CALLBACK( eos_cb ), NULL );

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
    if ( !gst_element_set_state( m_gst_thread, GST_STATE_PLAYING ) ) {
        warning() << "Could not set thread to PLAYING.\n";
        destroyPipeline();
        return false;
    }

    // If "Resume playback on start" is enabled, we must seek to the last position
    if ( offset ) seek( offset );

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
            startTimer( TIMER_INTERVAL );
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

    if ( GST_STATE( m_gst_thread ) == GST_STATE_PAUSED ) {
        gst_element_set_state( m_gst_thread, GST_STATE_PLAYING );
        emit stateChanged( Engine::Playing );
    }
    else {
        gst_element_set_state( m_gst_thread, GST_STATE_PAUSED );
        emit stateChanged( Engine::Paused );
    }
}


void
GstEngine::seek( uint ms )  //SLOT
{
    RETURN_IF_PIPELINE_EMPTY

    const int seekType = GST_FORMAT_TIME | GST_SEEK_METHOD_SET | GST_SEEK_FLAG_FLUSH;
    GstEvent* event = gst_event_new_seek( (GstSeekType) seekType, ms * GST_MSECOND );

    gst_element_send_event( m_gst_audiosink, event );
}


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


void
GstEngine::setEqualizerEnabled( bool enabled ) //SLOT
{
    m_equalizerEnabled = enabled;

    RETURN_IF_PIPELINE_EMPTY

    gst_element_set( m_gst_equalizer, "active", enabled, NULL );
}


void
GstEngine::setEqualizerParameters( int preamp, const QValueList<int>& bandGains ) //SLOT
{
    m_equalizerPreamp = preamp;
    m_equalizerGains = bandGains;

    RETURN_IF_PIPELINE_EMPTY

    // BEGIN Preamp
    gst_element_set( m_gst_equalizer, "preamp", ( preamp + 100 ) / 2 , NULL );
    // END

    // BEGIN Gains
    vector<int> gainsTemp;
    gainsTemp.resize( bandGains.count() );
    for ( uint i = 0; i < bandGains.count(); i++ )
        gainsTemp[i] = ( *bandGains.at( i ) + 100 ) / 2;

    gst_element_set( m_gst_equalizer, "gain", &gainsTemp, NULL );
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

    gst_element_set( m_gst_volume, "volume", (double) percent * fade * 0.01, NULL );
}


void GstEngine::timerEvent( QTimerEvent* )
{
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
        }
        setVolume( volume() );
    }
    else
        killTimers();
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


void
GstEngine::newMetaData()  //SLOT
{
    emit metaData( m_metaBundle );
}


void
GstEngine::kioFinished()  //SLOT
{
    DEBUG_FUNC_INFO

    // KIO::Job deletes itself when finished, so we need to zero the pointer
    m_transferJob = 0;

    // Tell streamsrc: This is the end, my friend
    m_streamBufStop = true;
}


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
    GList * pool_registries = NULL;
    GList* registries = NULL;
    GList* plugins = NULL;
    GList* features = NULL;
    QString name;
    QStringList results;

    pool_registries = gst_registry_pool_list ();
    registries = pool_registries;

    while ( registries ) {
        GstRegistry * registry = GST_REGISTRY ( registries->data );
        plugins = registry->plugins;

        while ( plugins ) {
            GstPlugin * plugin = GST_PLUGIN ( plugins->data );
            features = gst_plugin_get_feature_list ( plugin );

            while ( features ) {
                GstPluginFeature * feature = GST_PLUGIN_FEATURE ( features->data );

                if ( GST_IS_ELEMENT_FACTORY ( feature ) ) {
                    GstElementFactory * factory = GST_ELEMENT_FACTORY ( feature );

                    if ( g_strrstr ( factory->details.klass, classname ) ) {
                        name = g_strdup ( GST_OBJECT_NAME ( factory ) );
                        if ( name != "autoaudiosink" )
                            results << name;
                    }
                }
                features = g_list_next ( features );
            }
            plugins = g_list_next ( plugins );
        }
        registries = g_list_next ( registries );
    }
    g_list_free ( pool_registries );
    pool_registries = NULL;

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
    GError* err;
    if ( !( m_gst_audiosink = gst_parse_launch( output, &err ) ) ) {
        QTimer::singleShot( 0, this, SLOT( errorNoOutput() ) );
        return false;
    }

    m_gst_audiobin = gst_bin_new( "audiobin" );

    /* setting device property for AudioSink*/
    if ( GstConfig::useCustomSoundDevice() && !GstConfig::soundDevice().isEmpty() )
        gst_element_set( m_gst_audiosink, "device", GstConfig::soundDevice().latin1(), NULL );

    m_gst_equalizer = GST_ELEMENT( gst_equalizer_new() );
    gst_bin_add( GST_BIN( m_gst_audiobin ), m_gst_equalizer );
    if ( !( m_gst_audioconvert = createElement( "audioconvert", m_gst_audiobin ) ) ) { return false; }
    if ( !( m_gst_identity = createElement( "identity", m_gst_audiobin ) ) ) { return false; }
    if ( !( m_gst_volume = createElement( "volume", m_gst_audiobin ) ) ) { return false; }
    if ( !( m_gst_audioscale = createElement( "audioscale", m_gst_audiobin ) ) ) { return false; }

    g_signal_connect( G_OBJECT( m_gst_identity ), "handoff", G_CALLBACK( handoff_cb ), NULL );

    /* link elements */
    gst_element_link_many( m_gst_audioconvert, m_gst_equalizer, m_gst_identity,
                           m_gst_volume, m_gst_audioscale, m_gst_audiosink, NULL );

    gst_bin_add( GST_BIN( m_gst_audiobin ), m_gst_audiosink );
    gst_element_set_state( m_gst_audiobin, GST_STATE_PAUSED );

    m_pipelineFilled = true;
    return true;
}


void
GstEngine::destroyPipeline()
{
    DEBUG_BLOCK

    m_fadeValue = 0.0;

    // Clear the scope adapter
    m_mutexScope.lock();
    gst_adapter_clear( m_gst_adapter );
    m_mutexScope.unlock();

    if ( m_pipelineFilled ) {
        debug() << "Unreffing pipeline." << endl;
        gst_element_set_state( m_gst_thread, GST_STATE_NULL );
        gst_object_unref( GST_OBJECT( m_gst_thread ) );

        m_pipelineFilled = false;
    }

    // Destroy KIO transmission job
    if ( m_transferJob ) {
        m_transferJob->kill();
        m_transferJob = 0;
    }
}


void
GstEngine::sendBufferStatus()
{
    if ( m_streamBuffering ) {
        const int percent = (int) ( (float) m_streamBufIndex / STREAMBUF_MIN * 105.0 );
        emit statusText( i18n( "Buffering.. %1%" ).arg( MIN( percent, 100 ) ) );
    }
}


#include "gstengine.moc"
