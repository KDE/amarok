/***************************************************************************
gstengine.cpp - GStreamer audio interface

begin                : Jan 02 2003
copyright            : (C) 2003 by Mark Kretschmann
email                : markey@web.de
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "enginebase.h"
#include "gstengine.h"
#include "streamsrc.h"

#include <math.h>           //interpolate()
#include <vector>

#include <qfile.h>
#include <qmessagebox.h>    //fillPipeline()
#include <qstringlist.h>
#include <qtimer.h>

#include <kdebug.h>
#include <kurl.h>

#include <gst/gst.h>

using std::vector;


AMAROK_EXPORT_PLUGIN( GstEngine )

/////////////////////////////////////////////////////////////////////////////////////
// static
/////////////////////////////////////////////////////////////////////////////////////

static const int
SCOPEBUF_SIZE = 40000;

static const int
STREAMBUF_SIZE = 200000;

GError*
GstEngine::error_msg;

GstEngine*
GstEngine::self;


void
GstEngine::eos_cb( GstElement*, GstElement* )
{
    kdDebug() << k_funcinfo << endl;

    //this is the Qt equivalent to an idle function: delay the call until all events are finished,
    //otherwise gst will crash horribly
    QTimer::singleShot( 0, self, SLOT( stopAtEnd() ) );
}


void
GstEngine::handoff_cb( GstElement*, GstBuffer* buf, gpointer )
{
    int channels = 2;  //2 == default, if we cannot determine the value from gst
    GstCaps* caps = gst_pad_get_caps( gst_element_get_pad( self->m_spider, "src_0" ) );

    for ( int i = 0; i < gst_caps_get_size( caps ); i++ ) {
        GstStructure* structure = gst_caps_get_structure( caps, i );

        if ( gst_structure_has_field( structure, "channels" ) ) {
            //             kdDebug() << k_funcinfo << "Field 'channels' found." << endl;
            gst_structure_get_int( structure, "channels", &channels );
        }
    }
    gst_caps_free( caps );
    //     kdDebug() << k_funcinfo << "Channels: " << channels << endl;

    if ( GST_IS_BUFFER( buf ) ) {
        //         kdDebug() << k_funcinfo << "BUFFER_SIZE: " << GST_BUFFER_SIZE( buf ) << endl;
        gint16 * data = ( gint16* ) GST_BUFFER_DATA( buf );

        //divide length by 2 for casting from 8bit to 16bit, and divide by number of channels
        for ( ulong i = 0; i < GST_BUFFER_SIZE( buf ) / 2 / channels; i += channels ) {
            if ( self->m_scopeBufIndex == self->m_scopeBuf.size() ) {
                self->m_scopeBufIndex = 0;
                //                 kdDebug() << k_funcinfo << "m_scopeBuf overflow!\n";
            }

            float temp = 0.0;
            //add all channels together so we effectively get a mono scope
            for ( int j = 0; j < channels; j++ ) {
                //convert uint-16 to float and write into buf
                temp += ( float ) ( data[ i + j ] - 32768 ) / 32768.0;
            }
            self->m_scopeBuf[ self->m_scopeBufIndex++ ] = temp;
        }
    }
}


void
GstEngine::typefindFound_cb( GstElement* /*typefind*/, GstCaps* /*caps*/, GstElement* /*pipeline*/ )
{
    kdDebug() << "GstEngine::typefindFound" << endl;

    self->m_typefindResult = true;
}

//     const GList *elements = gst_registry_pool_feature_list( GST_TYPE_ELEMENT_FACTORY );
//
//     while ( elements != NULL )
//     {
//         factory = (GstElementFactory *) elements->data;
//         const gchar *klass = gst_element_factory_get_klass( factory );
//         elements = elements->next;
//     }


void
GstEngine::error_cb( GstElement* /*element*/, GstElement* /*source*/, GError* error, gchar* /*debug*/, gpointer /*data*/ )
{
    kdDebug() << k_funcinfo << endl;

    QTimer::singleShot( 0, self, SLOT( handleError() ) );
}


/////////////////////////////////////////////////////////////////////////////////////
// CLASS GSTENGINE
/////////////////////////////////////////////////////////////////////////////////////

GstEngine::GstEngine()
        : EngineBase()
        , m_thread( NULL )
        , m_scopeBufIndex( 0 )
        , m_streamBuf( new char[ STREAMBUF_SIZE ] )
        , m_pipelineFilled( false )
{
    kdDebug() << k_funcinfo << endl;
}


GstEngine::~GstEngine()
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    stop();
    cleanPipeline();
    delete[] m_streamBuf;

    kdDebug() << "END " << k_funcinfo << endl;
}


/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
/////////////////////////////////////////////////////////////////////////////////////

void
GstEngine::init( bool&, int scopeSize, bool )
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    self = this;
    m_mixerHW = -1;            //initialize

    m_scopeBuf.resize( SCOPEBUF_SIZE );
    m_scopeSize = 1 << scopeSize;

    gst_init( NULL, NULL );

    kdDebug() << "END " << k_funcinfo << endl;
}


bool
GstEngine::initMixer( bool hardware )
{
    closeMixerHW();

    if ( hardware )
        hardware = initMixerHW();

    return hardware;
}


bool
GstEngine::canDecode( const KURL &url, mode_t, mode_t )
{
    GstElement * pipeline, *filesrc, *typefind;
    m_typefindResult = false;

    /* create a new pipeline to hold the elements */
    pipeline = gst_pipeline_new( "pipeline" );

    /* create a disk reader */
    filesrc = gst_element_factory_make( "filesrc", "disk_source" );
    g_object_set( G_OBJECT( filesrc ), "location",
                  static_cast<const char*>( QFile::encodeName( url.path() ) ), NULL );

    typefind = gst_element_factory_make( "typefind", "typefind" );

    gst_bin_add_many( GST_BIN ( pipeline ), filesrc, typefind, NULL );

    gst_element_link_many( filesrc, typefind, NULL );

    g_signal_connect ( G_OBJECT( typefind ), "have-type",
                       G_CALLBACK( typefindFound_cb ), pipeline );

    gst_element_set_state( GST_ELEMENT( pipeline ), GST_STATE_PLAYING );

    while ( gst_bin_iterate ( GST_BIN ( pipeline ) ) );

    gst_element_set_state( GST_ELEMENT( pipeline ), GST_STATE_NULL );
    gst_object_unref( GST_OBJECT( pipeline ) );

    return m_typefindResult;
}


long
GstEngine::position() const
{
    if ( !m_pipelineFilled ) return 0;

    GstFormat fmt = GST_FORMAT_TIME;
    //value will hold the current time position in nanoseconds
    gint64 value = 0;
    gst_element_query( m_spider, GST_QUERY_POSITION, &fmt, &value );
   
    return static_cast<long>( ( value / GST_MSECOND ) ); // nanosec -> msec
}


EngineBase::EngineState
GstEngine::state() const
{
    if ( !m_pipelineFilled )
        return Empty;
    if ( m_playFlag )
        return Playing;

    switch ( gst_element_get_state( GST_ELEMENT( m_thread ) ) ) {
        case GST_STATE_NULL:
        return Empty;
        case GST_STATE_READY:
        return Idle;
        case GST_STATE_PLAYING:
        return Playing;
        case GST_STATE_PAUSED:
        return Paused;

        default:
        return Empty;
    }
}


bool
GstEngine::isStream() const
{
    return false;
}


vector<float>*
GstEngine::scope()
{
    vector<float>* scope = new vector<float>( m_scopeSize );

    interpolate( m_scopeBuf, *scope );
    m_scopeBufIndex = 0;

    return scope;
}


/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC SLOTS
/////////////////////////////////////////////////////////////////////////////////////

void
GstEngine::play( const KURL& url )              //SLOT
{
    stop();
    if ( m_pipelineFilled ) cleanPipeline();

    kdDebug() << "Sound output method: " << m_soundOutput << endl;

    /* create a new thread to hold the elements */
    m_thread = gst_thread_new ( "thread" );
    g_object_set( G_OBJECT( m_thread ), "priority", 2, NULL );

    m_audiosink = gst_element_factory_make( m_soundOutput.latin1(), "play_audio" );

    kdDebug() << "DefaultSoundDevice: " << ( ( m_defaultSoundDevice ) ? "true" : "false" ) << endl;
    kdDebug() << "Sound Device:       " << m_soundDevice << endl;

    /* setting device property for AudioSink*/
    if ( !m_defaultSoundDevice && !m_soundDevice.isEmpty() )
        g_object_set( G_OBJECT ( m_audiosink ), "device", m_soundDevice.latin1(), NULL );

    /* create source */
    if ( url.isLocalFile() ) {
        m_filesrc = gst_element_factory_make( "filesrc", "disk_source" );
        //load track into filesrc
        g_object_set( G_OBJECT( m_filesrc ), "location",
                      static_cast<const char*>( QFile::encodeName( url.path() ) ), NULL );
    } else {
        m_filesrc = GST_ELEMENT( gst_streamsrc_new( m_streamBuf, &m_streamBufIndex ) );
    }

    m_spider = gst_element_factory_make( "spider", "spider" );
    /* and an audio sink */
    m_identity = gst_element_factory_make( "identity", "rawscope" );
    m_volumeElement = gst_element_factory_make( "volume", "volume" );

    m_audioconvert = gst_element_factory_make( "audioconvert", "audioconvert" );
    m_audioscale = gst_element_factory_make( "audioscale", "audioscale" );

    g_signal_connect ( G_OBJECT( m_identity ), "handoff",
                       G_CALLBACK( handoff_cb ), m_thread );
    g_signal_connect ( G_OBJECT( m_audiosink ), "eos",
                       G_CALLBACK( eos_cb ), m_thread );
    //     g_signal_connect ( G_OBJECT( m_thread ), "error",
    //                        G_CALLBACK ( error_cb ), m_thread );

    /* add objects to the main pipeline */
    gst_bin_add_many( GST_BIN( m_thread ), m_filesrc, m_spider, m_identity,
                      m_volumeElement, m_audioconvert, m_audioscale, m_audiosink, NULL );
    /* link src to sink */
    gst_element_link_many( m_filesrc, m_spider, m_identity,
                           m_volumeElement, m_audioconvert, m_audioscale, m_audiosink, NULL );

    gst_element_set_state( GST_ELEMENT( m_thread ), GST_STATE_READY );

    m_pipelineFilled = true;
    m_streamBufIndex = 0;
    setVolume( volume() );

    if ( url.protocol() == "http" ) {
        kdDebug() << k_funcinfo << "Playing HTTP stream." << endl;
        m_playFlag = true;
    } else {
        play();
        m_playFlag = false;
    }
}


void
GstEngine::play()              //SLOT
{
    kdDebug() << k_funcinfo << endl;
    if ( !m_pipelineFilled ) return ;

    /* start playing */
    gst_element_set_state( GST_ELEMENT( m_thread ), GST_STATE_PLAYING );
}


void
GstEngine::stop()              //SLOT
{
    kdDebug() << k_funcinfo << endl;
    if ( !m_pipelineFilled ) return ;

    /* stop the thread */
    gst_element_set_state ( m_thread, GST_STATE_NULL );

    emit stopped();
}


void
GstEngine::pause()              //SLOT
{
    kdDebug() << k_funcinfo << endl;
    if ( !m_pipelineFilled ) return ;

    gst_element_set_state( GST_ELEMENT( m_thread ), GST_STATE_PAUSED );
}


void
GstEngine::seek( long ms )              //SLOT
{
    if ( !m_pipelineFilled ) return ;

    if ( ms > 0 )
    {
        GstEvent * event = gst_event_new_seek( ( GstSeekType ) ( GST_FORMAT_TIME |
                                               GST_SEEK_METHOD_SET |
                                               GST_SEEK_FLAG_FLUSH ),
                                               ms * GST_MSECOND );

        gst_element_send_event( m_audiosink, event );
    }
}


void
GstEngine::setVolume( int percent )              //SLOT
{
    m_volume = percent;

    if ( isMixerHardware() ) {
        EngineBase::setVolumeHW( percent );
        if ( m_pipelineFilled )
            g_object_set( G_OBJECT( m_volumeElement ), "volume", 1.0, NULL );
    } else {
        if ( m_pipelineFilled )
            // We're using a logarithmic function to make the volume ramp more natural
            g_object_set( G_OBJECT( m_volumeElement ), "volume",
                          ( double ) 1.0 - log10( ( 100 - percent ) * 0.09 + 1.0 ), NULL );
    }
}


void
GstEngine::newStreamData( char* buf, int size )             //SLOT
{
    if ( m_streamBufIndex + size > STREAMBUF_SIZE ) {
        size = STREAMBUF_SIZE - m_streamBufIndex;
        //         kdDebug() << "Stream buffer overflow!" << endl;
    }

    // Copy data into stream buffer
    memcpy( m_streamBuf + m_streamBufIndex, buf, size );
    // Adjust index
    m_streamBufIndex += size;

    // Wait until buffer is partly filled, then start playback
    if ( m_playFlag && m_streamBufIndex > STREAMBUF_SIZE / 2 ) {
        play();
        m_playFlag = false;
    }
}


/////////////////////////////////////////////////////////////////////////////////////
// PRIVATE SLOTS
/////////////////////////////////////////////////////////////////////////////////////

void
GstEngine::handleError()  //SLOT
{
    kdDebug() << "Error message: " << static_cast<const char*>( error_msg->message ) << endl;
}


void
GstEngine::stopAtEnd()              //SLOT
{
    kdDebug() << k_funcinfo << endl;
    if ( !m_pipelineFilled ) return ;

    /* stop the thread */
    gst_element_set_state ( m_thread, GST_STATE_READY );

    emit stopped();
}


/////////////////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
/////////////////////////////////////////////////////////////////////////////////////

QStringList
GstEngine::getPluginList( const QCString& classname )
{
    GList * pool_registries = NULL;
    GList* registries = NULL;
    GList* plugins = NULL;
    GList* features = NULL;
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

                    if ( g_strrstr ( factory->details.klass, classname ) )
                        results << g_strdup ( GST_OBJECT_NAME ( factory ) );
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


void
GstEngine::cleanPipeline()
{
    if ( m_pipelineFilled ) {
        gst_element_set_state ( m_thread, GST_STATE_NULL );
        gst_object_unref( GST_OBJECT( m_thread ) );
        m_pipelineFilled = false;
    }
}


void
GstEngine::interpolate( const vector<float> &inVec, vector<float> &outVec )
{
    double pos = 0.0;
    const double step = ( double ) m_scopeBufIndex / outVec.size();

    for ( uint i = 0; i < outVec.size(); ++i, pos += step ) {
        unsigned long index = ( unsigned long ) pos;

        if ( index >= m_scopeBufIndex )
            index = m_scopeBufIndex - 1;

        outVec[ i ] = inVec[ index ];
    }
}


#include "gstengine.moc"


