/***************************************************************************
                      gstengine.cpp  -  GStreamer audio interface
                         -------------------
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

#include <vector>

#include <qobject.h>
#include <qstring.h>
#include <qstringlist.h>

#include <kdebug.h>
#include <kurl.h>

#include <gst/gst.h>


AMAROK_EXPORT_PLUGIN( GstEngine )

//////////////////////////////////////////////////////////////////////
// STATIC METHODS
//////////////////////////////////////////////////////////////////////

void GstEngine::eos_cb( GstElement *typefind, GstElement *pipeline )
{
    kdDebug() << "GstEngine::eos_cb" << endl;

    gst_element_set_state( GST_ELEMENT( pGstEngine->m_pThread ), GST_STATE_READY );
    //     pGstEngine->emit endOfTrack();
}


void GstEngine::handoff_cb( GstElement *identity, GstBuffer *buf, GstElement *pipeline )
{
    kdDebug() << "GstEngine::handoff_cb" << endl;
    
    if ( GST_IS_BUFFER( buf ) )
    {
        kdDebug() << "GstEngine::handoff_cb BUFFER_SIZE: " << GST_BUFFER_SIZE( buf ) << endl;
        
        guint8* data = GST_BUFFER_DATA( buf );

        for ( ulong i = 0; i < GST_BUFFER_SIZE( buf ); ++i )
        {
            for ( ; pGstEngine->mCurrent < pGstEngine->mScopeEnd && i < GST_BUFFER_SIZE( buf ); ++pGstEngine->mCurrent, ++i )
            {
                *pGstEngine->mCurrent = (float) data[ i ] / 256.0;
            }
            
            if ( pGstEngine->mCurrent >= pGstEngine->mScopeEnd )
                pGstEngine->mCurrent = pGstEngine->mScope;
        }
    }
}


void GstEngine::typefindError_cb( GstElement *typefind, GstElement *pipeline )
{
    kdDebug() << "GstEngine::typefindError" << endl;

    gst_element_set_state( GST_ELEMENT( pipeline ), GST_STATE_NULL );
    gst_object_unref( GST_OBJECT( pipeline ) );
}


void GstEngine::typefindFound_cb( GstElement *typefind, GstCaps *caps, GstElement *pipeline )
{
    kdDebug() << "GstEngine::typefindFound" << endl;

    pGstEngine->m_typefindResult = true;
}

//     const GList *elements = gst_registry_pool_feature_list( GST_TYPE_ELEMENT_FACTORY );
//
//     while ( elements != NULL )
//     {
//         factory = (GstElementFactory *) elements->data;
//         const gchar *klass = gst_element_factory_get_klass( factory );
//         elements = elements->next;
//     }


/////////////////////////////////////////////////////////////////////////////////////
// CLASS GSTENGINE
/////////////////////////////////////////////////////////////////////////////////////

GstEngine::GstEngine()
        : EngineBase()
        , m_pThread( NULL )
        , mScope( 0 )
{}


GstEngine::~GstEngine()
{
    stop();
    gst_object_unref      (GST_OBJECT  (m_pThread));
    delete [] mScope;
}


void GstEngine::init( bool&, int scopeSize, bool )
{
    m_mixerHW = -1;            //initialize 
    buffer( 1 << scopeSize );
    pGstEngine = this;
    
//     g_module_open( "libgstreamer-0.8.so", (GModuleFlags) 0 );
    gst_init( NULL, NULL );
    
    /* create a new thread to hold the elements */
    kdDebug() << k_funcinfo << "BEFORE gst_thread_new ( thread );\n";
    m_pThread              = gst_thread_new          ( "thread" );
    
    /* create a disk reader */
    kdDebug() << k_funcinfo << "BEFORE gst_element_factory_make( filesrc, disk_source );\n";
    m_pFilesrc             = gst_element_factory_make( "filesrc", "disk_source" );
    GstElement *spider     = gst_element_factory_make( "spider", "spider" );
    /* and an audio sink */
    
    kdDebug() << k_funcinfo << "BEFORE gst_element_factory_make( osssink, play_audio );\n";
    m_pAudiosink           = gst_element_factory_make( "osssink", "play_audio" );
    GstElement *pIdentity  = gst_element_factory_make( "identity", "rawscope" );

//     g_signal_connect ( G_OBJECT( m_pAudiosink ), "handoff",
//                        G_CALLBACK( handoff_cb ), m_pThread );

    g_signal_connect ( G_OBJECT( pIdentity ), "handoff",
                       G_CALLBACK( handoff_cb ), m_pThread );

    g_signal_connect ( G_OBJECT( m_pAudiosink ), "eos",
                       G_CALLBACK( eos_cb ), m_pThread );

    /* add objects to the main pipeline */
    gst_bin_add_many( GST_BIN( m_pThread ), m_pFilesrc, spider, pIdentity, m_pAudiosink, NULL );
    /* link src to sink */
    
    gst_element_link_many( m_pFilesrc, spider, pIdentity, m_pAudiosink, NULL );
}


/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
/////////////////////////////////////////////////////////////////////////////////////

bool GstEngine::initMixer( bool )
{
    closeMixerHW();
    initMixerHW();
    
    setVolume( m_volume );

    return true;
}


bool GstEngine::canDecode( const KURL &url, mode_t, mode_t )
{
    GstElement *pipeline, *filesrc, *typefind;
    m_typefindResult = false;

    /* create a new thread to hold the elements */
    pipeline = gst_pipeline_new( "pipeline" );

    /* create a disk reader */
    filesrc = gst_element_factory_make( "filesrc", "disk_source" );
    g_object_set( G_OBJECT( filesrc ), "location", url.path().latin1(), NULL );

    typefind = gst_element_factory_make( "typefind", "typefind" );

    gst_bin_add_many( GST_BIN ( pipeline ), filesrc, typefind, NULL );

    gst_element_link_many( filesrc, typefind, NULL );

    g_signal_connect ( G_OBJECT( typefind ), "have-type",
                       G_CALLBACK( typefindFound_cb ), pipeline );

    //     g_signal_connect ( G_OBJECT( typefind ), "error",
    //                        G_CALLBACK( typefindError_cb ), pipeline );

    gst_element_set_state( GST_ELEMENT( pipeline ), GST_STATE_PLAYING );

    while( gst_bin_iterate ( GST_BIN ( pipeline ) ) );

    gst_element_set_state( GST_ELEMENT( pipeline ), GST_STATE_NULL );
    gst_object_unref( GST_OBJECT( pipeline ) );

    return m_typefindResult;
}


long GstEngine::position() const
{
    if ( ( gst_element_get_state( GST_ELEMENT (m_pThread) ) == GST_STATE_PLAYING ) ||
            ( gst_element_get_state( GST_ELEMENT (m_pThread) ) == GST_STATE_PAUSED ) )
    {
        GstClock *clock;
        clock = gst_bin_get_clock ( GST_BIN ( m_pThread ) );

        return (long) ( gst_clock_get_time ( clock ) / GST_MSECOND );
    }
    else
        return 0;
}


EngineBase::EngineState GstEngine::state() const
{
    switch ( gst_element_get_state( GST_ELEMENT( m_pThread ) ) )
    {
        case GST_STATE_NULL:
            return Empty;
        case GST_STATE_READY:
            return Playing;
        case GST_STATE_PLAYING:
            return Playing;
        case GST_STATE_PAUSED:
            return Paused;

        default:
            return Empty;
    }
}


bool GstEngine::isStream() const
{
    return false;
}


std::vector<float>* GstEngine::scope()
{
    std::vector<float> *buf = new std::vector<float>( mScopeLength );
    char *front = (char *)(&buf->front());
    memcpy(front, mCurrent, (mScopeEnd - mCurrent) * sizeof(float));
    memcpy(front + (mScopeEnd - mCurrent)*sizeof(float), mScope,
           (mCurrent - mScope) * sizeof(float));

    return buf;
}


/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC SLOTS
/////////////////////////////////////////////////////////////////////////////////////

const QObject* GstEngine::play( const KURL& url )
{
    stop();

    g_object_set( G_OBJECT( m_pFilesrc ), "location", url.path().latin1(), NULL );
    play();

    return 0;
}


void GstEngine::play()
{
    /* start playing */
    gst_element_set_state( GST_ELEMENT( m_pThread ), GST_STATE_PLAYING );
}


void GstEngine::stop()
{
    /* stop the thread */
    gst_element_set_state (GST_ELEMENT( m_pThread ), GST_STATE_NULL );
}


void GstEngine::pause()
{
    gst_element_set_state( GST_ELEMENT( m_pThread ), GST_STATE_PAUSED );
}


void GstEngine::seek( long ms )
{
    if ( ms > 0 )
    {
        GstEvent *event = gst_event_new_seek( (GstSeekType) ( GST_FORMAT_TIME |
                                              GST_SEEK_METHOD_SET |
                                              GST_SEEK_FLAG_FLUSH ),
                                              ms * GST_MSECOND );

        gst_element_send_event( m_pAudiosink, event );
    }
}


void GstEngine::setVolume( int percent )
{
    m_volume = percent;
    EngineBase::setVolumeHW( percent );
}


/////////////////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
/////////////////////////////////////////////////////////////////////////////////////

void GstEngine::buffer( long len )
{
    delete [] mScope;

    mScopeLength=len;
    mScope=new float[len];
    mScopeEnd=mScope+mScopeLength;
    mCurrent=mScope;

    memset(mScope, 0, mScopeLength);
}


#include "gstengine.moc"

