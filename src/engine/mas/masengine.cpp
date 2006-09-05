/***************************************************************************
         masengine.cpp  -  MAS audio interface
                         -------------------
begin                : 2004-07-20
copyright            : (C) 2004-05 by Roland Gigler
email                : rolandg@web.de
what                 : interface to the Media Application Server (MAS)
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define DEBUG_PREFIX "MAS-Engine"

#include "debug.h"
#include "enginebase.h"
#include "engineobserver.h"

#include <assert.h>
#include <math.h>            //setVolume(), timerEvent()
#include <string>
//#include <vector>

#include <qtimer.h>
#include <qfile.h>

#include <kapplication.h>
#include <kconfig.h>
#include <kfileitem.h>
#include <kgenericfactory.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetype.h>
#include <kstandarddirs.h>
#include <kurl.h>

#include "masengine.h"
//#define DB_CUTOFF -40.0
#define BUFFER_TIME_MS 300
#define POSTOUT_TIME_MS 100
//#define QUERY_MIX_VOLUME 0
//#define QUERY_MIX_EPSILON 5



AMAROK_EXPORT_PLUGIN( MasEngine )


MasEngine::MasEngine()
        : Engine::Base()
        , m_inited (false)
//        , m_scopeId( 0 )
//        , m_xfadeFadeout( false )
//        , m_xfadeValue( 0.0 )
//        , m_xfadeCurrent( "invalue2" )
        , m_lastKnownPosition( 0 )
        , m_state( Engine::Empty )
        , m_pPlayingTimer( new QTimer( this ) )

{
    DEBUG_FUNC_INFO

    // NOT SUPPORTED
    //addPluginProperty( "HasConfigure",   "true" );
    //addPluginProperty( "HasCrossfading", "true" );
    //addPluginProperty( "HasEqualizer",   "true" );
}


MasEngine::~MasEngine()
{
    DEBUG_BLOCK

    if ( m_inited ) stop();
    m_pPlayingTimer->stop();
    killTimers();
}


bool MasEngine::init()
{
    DEBUG_BLOCK

    if (!masinit() ) {
        KMessageBox::error( 0, i18n("<h3>Amarok could not initialise MAS.</h3>"
                        "<p>Check for a running mas daemon.</p>") );
        error() << "  connecting to MAS daemon failed. Aborting. " << endl;
        debug() << "  Please restart amarok." << endl;
        debug() << k_funcinfo << "  returns false !" << endl;
        return false;
    }
    m_inited=true;              // we connected to MAS

    connect ( m_pPlayingTimer, SIGNAL( timeout() ), this, SLOT( playingTimeout() ) );

    emit statusText( "MAS Engine inited :-)");
    return true;
}


////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
////////////////////////////////////////////////////////////////////////////////

bool MasEngine::canDecode( const KURL &url ) const
{
    DEBUG_BLOCK

    QStringList list;
    bool playable;

    debug() << "  Param: url: " << url << endl;
    //debug() << "  url.protocol()   >" << url.protocol() <<"<"<< endl;

    if (url.protocol() == "http" ) return false;

    // TODO determine list of supported MimeTypes/Extensions from MAS
    list += QString("audio/x-mp3");

    KFileItem fileItem( KFileItem::Unknown, KFileItem::Unknown, url, false ); //false = determineMimeType straight away
    KMimeType::Ptr mimetype = fileItem.determineMimeType();
    debug() << "mimetype: " << mimetype->name().latin1() << endl;

    playable = list.contains( mimetype->name().latin1() );
    if ( !playable )
      warning() << "Mimetype is not playable by MAS (" << url << ")" << endl;

    return playable;
}  // canDecode


bool MasEngine::load( const KURL& url, bool stream )
{
    DEBUG_BLOCK
    struct mas_package pkg;
    char pbuf[10240];
    int pos = 0;

    m_isStream = stream;
    debug() << "  m_url: " << m_url << endl;
    debug() << "  Param: stream: " << stream << endl;
    debug() << "  Param: url " << url << endl;

    if ( !url.isLocalFile() ) {         // for now
        debug() << "  only local files are supported (for now)" << endl;
        return false;
    }

    if ( !canDecode(url) ) {
        debug() << "  cannot decode!" << endl;
        return false;
    }

    if ( m_url == url ) {
       return true;
    } else {
       stop();
   }
    m_url = url;

    /* send fresh data to MAS */;
    masc_setup_package( &pkg, pbuf, sizeof pbuf, MASC_PACKAGE_STATIC );
    masc_pushk_int16( &pkg, (char*)"pos", pos );
    //masc_push_string( &pkg, (char *)m_url.path().latin1() );

    QCString cs= QFile::encodeName( m_url.path());
    const char *pcs = cs;
    masc_push_string( &pkg, (char *)pcs);
    masc_finalize_package( &pkg );

    mas_set( m_mp1a_source_device, (char*)"playlist", &pkg );
    masc_strike_package( &pkg );

    mas_dev_show_state (m_mp1a_source_device);
    mas_source_flush( m_codec );

    m_lastKnownPosition = 0;
    m_state = Engine::Idle;

    return true;
}   // load


bool MasEngine::play( unsigned int offset)
{
    DEBUG_BLOCK
    struct mas_package pkg;
    char pbuf[10240];

    debug() << "  param: offset " << offset << endl;
    if ( m_state != Engine::Playing ) {
        /* change the track */
        masc_setup_package( &pkg, pbuf, sizeof pbuf, MASC_PACKAGE_STATIC );
        masc_pushk_int16( &pkg, (char*)"pos", 1 );
        masc_finalize_package( &pkg );
        mas_set( m_mp1a_source_device, (char*)"ctrack", &pkg );
        masc_finalize_package( &pkg );

        mas_source_flush( m_codec );
        mas_source_play( m_mp1a_source_device );
        mas_source_play_on_mark( m_sbuf );

        debug() << "mas_source_play()" << endl;
    }

    m_pPlayingTimer->start(MAS_TIMER, false);

    m_state = Engine::Playing;
    emit stateChanged( Engine::Playing );

    return true;
}   // play
uint
MasEngine::length() const
{
    DEBUG_BLOCK
    char pbuf[128];
    struct mas_package pkg;
    struct mas_package nugget;
    float trklen;

    masc_setup_package( &pkg, pbuf, sizeof pbuf, MASC_PACKAGE_STATIC );
    masc_pushk_int16( &pkg, (char *)"pos", 1 );
    masc_finalize_package( &pkg );

    mas_get( m_mp1a_source_device, (char *)"trklen", &pkg, &nugget );
    masc_strike_package( &pkg );

    masc_pullk_float( &nugget, (char *)"trklen", &trklen );
    masc_strike_package( &nugget );

    debug() << "trklen: " <<  trklen << endl;
    return uint(trklen*1000);
}   // position

/*
 * return current position in milli seconds
*/
uint MasEngine::position() const
{
    return m_lastKnownPosition;
}   // position


//////////////////////////////////////////////////////////////////////


void MasEngine::playingTimeout() //SLOT
{
    //DEBUG_BLOCK
    m_lastKnownPosition += MAS_TIMER;

    // ask MAS if it's still playing
    struct mas_package nugget;
    int16 pos;
    mas_get( m_mp1a_source_device, (char *)"ctrack", NULL, &nugget );
    masc_pullk_int16( &nugget, (char *)"pos", &pos );
    masc_strike_package( &nugget );

    if ( pos == 0 ) {
        m_pPlayingTimer->stop();

        m_state = Engine::Idle;
        emit trackEnded();
    }

}   // playingTimeout    //SLOT


void MasEngine::stop()
{
    DEBUG_BLOCK

    //switch xfade channels
/*    m_xfadeCurrent = ( m_xfadeCurrent == "invalue1" ) ? "invalue2" : "invalue1";

    if ( m_xfadeValue == 0.0 )
        m_xfadeValue = 1.0;
*/
    mas_source_stop( m_mp1a_source_device );
    mas_source_stop( m_sbuf );
    debug() << "performed: mas_source_stop()" << endl;

    m_pPlayingTimer->stop();
    m_state = Engine::Empty;
    m_lastKnownPosition = 0;

    //emit stateChanged( m_state );
}


void MasEngine::pause()
{
    DEBUG_BLOCK

    if(m_state == Engine::Paused) {
        mas_source_play( m_mp1a_source_device );
        mas_source_play( m_sbuf );
        debug() << "performed: mas_source_play()" << endl;

        m_state = Engine::Playing;
        m_pPlayingTimer->start(MAS_TIMER, false);

    } else {
        mas_source_pause( m_mp1a_source_device );
        mas_source_pause( m_sbuf );
        debug() << "performed: mas_source_pause()" << endl;

        m_state = Engine::Paused;
        m_pPlayingTimer->stop();
    }

    emit stateChanged( m_state );
}   // pause


// TODO depends on MAS support
void MasEngine::seek( unsigned int ms )
{
    //DEBUG_BLOCK
    AMAROK_NOTIMPLEMENTED
    //debug() << "  param: ms " << ms << endl;
}   // seek


void MasEngine::setVolumeSW( unsigned int percent )
{
    DEBUG_BLOCK
    debug() << "  Param: percent " << percent << endl;

#ifdef USE_MIX_VOLUME_GENERIC
    // MAS takes values from 0 to 128
    int16 vol = (int16)(percent*1.28);
    debug() << " setting vol to " << vol << endl;

    // the more generic way
    struct mas_package pkg;
    char buffer[128];

    masc_setup_package( &pkg, buffer, sizeof buffer, MASC_PACKAGE_STATIC );
    masc_push_int32( &pkg, m_mix_sink->portnum );
    masc_push_uint16( &pkg, vol );
    masc_finalize_package( &pkg );

    mas_set( m_mix_device, (char *)"multiplier", &pkg );
    masc_strike_package( &pkg );
    debug() << "  after mas_set" << endl;
#else
    // use the mix api
    double vol = percent*0.01;
    debug() << " setting vol to " << vol << endl;
    mas_mix_attenuate_linear( m_mix_device, m_mix_sink, vol);
#endif

}   // setVolumeSW

////////////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
////////////////////////////////////////////////////////////////////////////////
/*
void MasEngine::timerEvent( QTimerEvent* )
{
    if ( m_xfadeValue > 0.0 )
    {
        m_xfadeValue -= ( m_xfadeLength ) ?  1.0 / m_xfadeLength * ARTS_TIMER : 1.0;

        if ( m_xfadeValue <= 0.0 )
        {
            m_xfadeValue = 0.0;
        }
        float value;
        if ( m_xfadeFadeout )
            value = 1.0 - log10( ( 1.0 - m_xfadeValue ) * 9.0 + 1.0 );
        else
            value = log10( m_xfadeValue * 9.0 + 1.0 );

    }
}
*/

bool MasEngine::masinit()
{
    DEBUG_BLOCK
    int32 err;

    masc_log_verbosity( MAS_VERBLVL_DEBUG );
    masc_log_message( 0, (char*)"amarok/MASengine" );
    masc_log_message( 0, (char*)"tries to plays audio using MAS ;-)");
    masc_log_message( 0, (char*)"" );

    err = mas_init();
    debug() << "  mas_init err:" << err << endl;
    if (err < 0)
    {
        warning() << "MAS daemon not running. Starting! Please provide password..." << endl;
        // masd seems not to be running, let's try to run it
        QCString cmdline;
        cmdline = QFile::encodeName( KStandardDirs::findExe( QString::fromLatin1( "kdesu" ) ) );
        // TODO !!!hardcoded path
        cmdline += " -n -f /usr/local/mas/log/mas-1.log -c ";
        cmdline += "/usr/local/mas/bin/mas-launch";

        debug() << "  cmdline: " << cmdline << endl;
        int status = ::system( cmdline );
        debug() << "  status: " << status << endl;

        debug() << "  give the MAS daemon some time (3s) ... " << endl;
        ::sleep( 3 );

        if ( status != -1 && WIFEXITED( status ) )
        {
            int time = 0;
            do
            {
                debug() << "  time: " << time << endl;
                // every time it fails, we should wait a little longer
                // between tries
                ::sleep( 1 + time / 2 );
                err = mas_init();
                debug() << "  mas_init err: " << err << endl;
            }
            while ( ++time < 5 && ( err < 0 ) );
        }
    }

    if (err < 0)
    {
        QString text = "Connection to MAS daemon FAILED. Please check your installation.";
        error() << text << endl;
        emit statusText( text );
        return false;
    }
    debug() << "Connection to MAS daemon is up :-)" << endl;

    struct mas_data_characteristic* dc;
    struct mas_package nugget;
    mas_device_t anx;
    mas_port_t   tmp_source;
    mas_channel_t local;

    err = mas_get_local_control_channel( &local );
    if ( err < 0 ) masc_logerror( err, "getting local control channel" );
    debug() << "  after mas_get_local_control_channel" << endl;

    /* Get the id of the sample clock provided by the anx device -- if
       we can! */
    err = mas_asm_get_device_by_name( (char*)"anx", &anx );
    mas_assert( err >= 0, (char*)"Couldn't get anx device" );
    debug() << "  after mas_asm_get_device_by_name" << endl;

    m_sink_clkid = 0;
    err = mas_get( anx, (char*)"mc_clkid", NULL, &nugget );
    if ( err >= 0 )
    {
        masc_pull_int32( &nugget, &m_sink_clkid );
        masc_strike_package( &nugget );
    }
    debug() <<"  after mas_get (mc_clkid)" << endl;

    /* CODEC */
    err = mas_asm_instantiate_device( (char*)"codec_mp1a_mad", 0, 0, &m_codec );
    if (err < 0 )
    {
        masc_log_message( MAS_VERBLVL_INFO, (char*)"Couldn't instantiate mp1a_mad codec, trying mp1a codec.");
        err = mas_asm_instantiate_device( (char*)"codec_mp1a", 0, 0, &m_codec );
        mas_assert( err >= 0, (char*)"Couldn't instantiate MPEG codec");
    }
    debug() << "  after mas_asm_instantiate_device (codec_mp1a_mad)"  << endl;

    /* source - instantiated on the local MAS server */
    err = mas_asm_instantiate_device_on_channel( (char*)"source_mp1a", 0, 0, &m_mp1a_source_device, local );
    mas_assert( err >= 0, (char*)"Couldn't instantiate source_mp1a" );

    /* buffer */
    err = mas_asm_instantiate_device( (char*)"sbuf", 0, 0, &m_sbuf );
    mas_assert( err >= 0, (char*)"Couldn't instantiate sbuf device" );
    debug() << "  after mas_asm_instantiate_device (sbuf)" << endl;

    m_visual = NULL;
#ifdef USE_VISUAL
    err = mas_asm_instantiate_device( (char*)"visual", 0, 0, &m_visual );
    if ( err < 0 )
    {
        masc_log_message( 0, (char*)"Couldn't instantiate visual device.  It's okay, I just won't use it." );
        m_visual = NULL;
    }
    debug() << "  after mas_asm_instantiate_device (visual)" << endl;
#endif


    /* get a handle to the mixer */
    err = mas_asm_get_device_by_name( (char*)"mix", &m_mix_device );
    mas_assert( err >= 0, (char*)"Couldn't get mixer device" );
    debug() << "  after mas_asm_get_device_by_name (mix)" << endl;

    /* start making connections:
     *
     *    source->codec->visual->mix
     */

    err = mas_asm_connect_devices( m_mp1a_source_device, m_codec, (char*)"source", (char*)"sink" );
    mas_assert( err >= 0, "Couldn't connect MPEG source to MPEG codec" );

    dc = masc_make_audio_basic_dc( MAS_LINEAR_FMT, 44100, 20, 2, MAS_HOST_ENDIAN_FMT );
    mas_assert( dc, "Couldn't create audio data characteristic." );

    err = mas_asm_get_port_by_name( m_mix_device, (char*)"default_mix_sink", &m_mix_sink );
    mas_assert( err >= 0, "Couldn't get default mixer sink" );
    debug() << "  m_mix_sink:" << m_mix_sink << endl;

    err = mas_asm_connect_devices_dc( m_codec, m_sbuf, (char*)"source", (char*)"sink", dc );
    mas_assert( err >= 0, "Couldn't connect MPEG codec to sbuf" );
    debug() << "  after mas_asm_connect_device_dc (source, sink)" << endl;

    if (m_visual != NULL)
    {
        err = mas_asm_connect_devices_dc( m_sbuf, m_visual, (char*)"source", (char*)"sink", dc );
        mas_assert( err >= 0, "Couldn't connect sbuf to visual device." );

        err = mas_asm_get_port_by_name( m_visual, (char*)"source", &tmp_source );
        mas_assert( err >= 0, "Couldn't get visual source port" );

        err = mas_asm_connect_source_sink( tmp_source, m_mix_sink, dc );
        mas_assert( err >= 0, "Couldn't connect visual device to mixer sink." );
    }
    else
    {
        err = mas_asm_get_port_by_name( m_sbuf, (char*)"source", &tmp_source );
        mas_assert( err >= 0, "Couldn't get sbuf source port" );

        err = mas_asm_connect_source_sink( tmp_source, m_mix_sink, dc );
        mas_assert( err >= 0, "Couldn't connect sbuf to mix device." );
    }
    debug() << "  after ... visual ..." << endl;

    /* set the buffer time */
    masc_setup_package( &nugget, NULL, 0, 0 );
    masc_pushk_uint32( &nugget, (char*)"buftime_ms", BUFFER_TIME_MS );
    masc_finalize_package( &nugget );
    mas_set( m_sbuf, (char*)"buftime_ms", &nugget );
    masc_strike_package( &nugget );

    debug() << "  after ... masc_strike_package ..." << endl;

    masc_setup_package( &nugget, NULL, 0, 0 );
    masc_pushk_uint32( &nugget, "postout_time_ms", POSTOUT_TIME_MS );
    masc_finalize_package( &nugget );
    mas_set( m_sbuf, "postout_time_ms", &nugget );
    masc_strike_package( &nugget );

    /* If we can use a sample clock, let's do it... */
    if ( m_sink_clkid > 0 )
    {
        mas_get_mc_device( &m_sink_mc );
        mas_get_mc_device_on_channel( &m_source_mc, local );
        debug() << "  after ... masc_get_mc_device_on_channel ..." << endl;

        //get_measured_sample_freq();
/*
        m_source_clkid = mas_mc_add_fixed_clock( "player", m_measured_sample_freq, local );
    printf ("  after ... masc_mc_add_fixed_clock ...\n");
        masc_log_message( 0, "got clock: %d", m_source_clkid );
 */
        /* the file source device uses the file clock */
/*        masc_setup_package( &nugget, NULL, 0, 0 );
        masc_pushk_int32( &nugget, "mc_clkid", m_source_clkid );
        masc_finalize_package( &nugget );
        mas_set( mp1a_source_device, "mc_clkid", &nugget );
        masc_strike_package( &nugget );
*/
        /* and the sbuf, which could be on a different machine, uses
           the sample clock from that machine's anx device. */
/*        masc_setup_package( &nugget, NULL, 0, 0 );
        masc_pushk_int32( &nugget, "mc_clkid", m_sink_clkid );
        masc_finalize_package( &nugget );
        mas_set( m_sbuf, "mc_clkid", &nugget );
        masc_strike_package( &nugget );
*/
    }

    return true;
}   // masinit

#include "masengine.moc"

