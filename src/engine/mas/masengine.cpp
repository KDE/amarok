/***************************************************************************
         masengine.cpp  -  MAS audio interface
                         -------------------
begin                : 2004-07-20
copyright            : (C) 2004 by Roland Gigler
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

#include "enginebase.h"
#include "engineobserver.h"

#include <assert.h>
#include <math.h>            //setVolume(), timerEvent()
#include <string>
#include <vector>

#include <qdir.h>
#include <qdom.h>
#include <qfile.h>
#include <qlayout.h>
#include <qstring.h>
#include <qtextstream.h>
#include <qtimer.h>

#include <kapplication.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kfileitem.h>
#include <kgenericfactory.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetype.h>
#include <kstandarddirs.h>
#include <kurl.h>

#include "masengine.h"
#define DB_CUTOFF -40.0
#define BUFFER_TIME_MS 300
/* #define POSTOUT_TIME_MS 100 */
#define QUERY_MIX_VOLUME 0
#define QUERY_MIX_EPSILON 5



AMAROK_EXPORT_PLUGIN( MasEngine )


MasEngine::MasEngine( )
        : EngineBase()
        , m_scopeId( 0 )
        , m_volumeId( 0 )
//        , m_xfadeFadeout( false )
//        , m_xfadeValue( 0.0 )
//        , m_xfadeCurrent( "invalue2" )
        , m_lastKnownPosition( 0 )
        , m_state( Engine::Empty )
        , m_pPlayingTimer( new QTimer( this ) )

{
    kdDebug() << k_funcinfo << endl;
}


MasEngine::~MasEngine()
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    stop();
    m_pPlayingTimer->stop();
    killTimers();

    kdDebug() << "END " << k_funcinfo << endl;
}


bool MasEngine::init()
{

    kdDebug() << "BEGIN " << k_funcinfo << endl;
   /*
    m_scopeSize = 1 << scopeSize;
    m_restoreEffects = restoreEffects;
    m_mixerHW = -1;   //initialize
    */

    if (!masinit() ) {
        KMessageBox::error( 0, i18n("amaroK could not initialise MAS. Check for a running mas daemon.") );
        kdDebug() << "  connecting to mas daemon failed. Aborting. " << endl;
        kdDebug() << k_funcinfo << "  returns FALSE !" << endl;
        return false;
    }

    connect ( m_pPlayingTimer, SIGNAL( timeout() ), this, SLOT( playingTimeout() ) );

    kdDebug() << "END " << k_funcinfo << endl;
    return true;
}

#ifdef UNUSED
bool MasEngine::initMixer( bool hardware )
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    kdDebug() << "  Param: hardware " << hardware << endl;
    { //make sure any previously started volume control gets killed
        if ( m_volumeId )
        {
            m_volumeId = 0;
        }
        closeMixerHW();
    }

    if ( hardware )
        hardware = initMixerHW();
    else
    {

    }
    kdDebug() << "END " << k_funcinfo << endl;
    return true;
}
#endif

////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
////////////////////////////////////////////////////////////////////////////////

bool MasEngine::canDecode( const KURL &url ) const
{
    static QStringList list;

    kdDebug() << "BEGIN " << k_funcinfo << endl;
    kdDebug() << "  Param: url: " << url << endl;

    // TODO determine list of supported MimeTypes/Extensions from MAS
    list += QString("audio/x-mp3");

    KFileItem fileItem( KFileItem::Unknown, KFileItem::Unknown, url, false ); //false = determineMimeType straight away
    KMimeType::Ptr mimetype = fileItem.determineMimeType();
    kdDebug() << "mimetype: " << mimetype->name().latin1() << endl;

    return list.contains( mimetype->name().latin1() );
}  // canDecode


bool MasEngine::load( const KURL& url, bool stream )
{
    struct mas_package pkg;
    char pbuf[10240];
    int pos = 0;

    kdDebug() << "BEGIN " << k_funcinfo << endl;

    m_isStream = stream;
    kdDebug() << "  m_url(1) " << m_url << endl;


    kdDebug() << "  Param: url " << url << endl;
    kdDebug() << "  url.path()   >" << url.path() <<"<"<< endl;
    kdDebug() << "  url.protocol >" << url.protocol() <<"<"<< endl;
    kdDebug() << "  url.host     >" << url.host() <<"<"<< endl;
    kdDebug() << "  url.port     >" << url.port() <<"<"<< endl;
    kdDebug() << "  url.isLocalFile()     >" << url.isLocalFile() <<"<"<< endl;
    kdDebug() << "  url.htmlURL()     >" << url.htmlURL() <<"<"<< endl;
    kdDebug() << "  url.prettyURL()     >" << url.prettyURL() <<"<"<< endl;
    kdDebug() << "  url.isValid()     >" << url.isValid() <<"<"<< endl;

    if ( m_url == url ) {
       return true;
    } else {
       stop();
   }
    m_url = url;
    kdDebug() << "  m_url(2) " << m_url << endl;

    /* send fresh data to MAS */;
    masc_setup_package( &pkg, pbuf, sizeof pbuf, MASC_PACKAGE_STATIC );
    masc_pushk_int16( &pkg, (char*)"pos", pos );
    masc_push_string( &pkg, (char *)m_url.path().latin1() );

    QCString cs= QFile::encodeName( m_url.path());
    kdDebug() << "  cs " << cs << endl;
    //char *x = cs;
    //masc_push_string( &pkg, x);
    masc_finalize_package( &pkg );

    mas_set( m_mp1a_source_device, (char*)"playlist", &pkg );
    masc_strike_package( &pkg );
    printf( "pkg: loaded mas with playlist: %s\n", (char *)m_url.path().latin1() );

    //mas_dev_show_state (m_mp1a_source_device);
    //mas_source_flush( m_codec );

    m_lastKnownPosition = 0;

    kdDebug() << "END " << k_funcinfo << endl;
    return true;
}   // load


bool MasEngine::play( unsigned int offset)
{
    struct mas_package pkg;
    char pbuf[10240];
    kdDebug() << "BEGIN " << k_funcinfo << endl;
    kdDebug() << "  param: offset " << offset << endl;

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

        printf( "mas_source_play()\n" );
    }

    m_pPlayingTimer->start(MAS_TIMER, false);

    m_state = Engine::Playing;
    emit stateChanged( Engine::Playing );

    kdDebug() << "END " << k_funcinfo << endl;
    return true;
}   // play


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
   // kdDebug() << "BEGIN " << k_funcinfo << endl;
    m_lastKnownPosition += MAS_TIMER;

    // ask MAS if it's still playing
    struct mas_package nugget;
    int16 pos;
    mas_get( m_mp1a_source_device, (char *)"ctrack", NULL, &nugget );
    masc_pullk_int16( &nugget, (char *)"pos", &pos );
    masc_strike_package( &nugget );
    //masc_log_message( 0, (char*)"after calling source ctrack: pos %d", pos);

    if ( pos == 0 ) {
        m_pPlayingTimer->stop();

        m_state = Engine::Idle;
        emit trackEnded( );
    }

    //kdDebug() << "END " << k_funcinfo << endl;
}   // playingTimeout

//SLOT


void MasEngine::stop()
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    //switch xfade channels
/*    m_xfadeCurrent = ( m_xfadeCurrent == "invalue1" ) ? "invalue2" : "invalue1";

    if ( m_xfadeValue == 0.0 )
        m_xfadeValue = 1.0;
*/
    mas_source_stop( m_mp1a_source_device );
    mas_source_stop( m_sbuf );
    kdDebug() << "performed: mas_source_stop()" << endl;

    m_pPlayingTimer->stop();

    m_state = Engine::Empty;
    //m_state = Engine::Idle;

    m_lastKnownPosition = 0;

    kdDebug() << "END " << k_funcinfo << endl;
}


void MasEngine::pause()
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;

    if(m_state == Engine::Paused) {
        mas_source_play( m_mp1a_source_device );
        mas_source_play_on_mark( m_sbuf );

        m_state = Engine::Playing;
        m_pPlayingTimer->start(MAS_TIMER, false);

    } else {
        mas_source_pause( m_mp1a_source_device );
        mas_source_pause( m_sbuf );
        printf( "performed:\nmas_source_pause()\n" );

        m_state = Engine::Paused;
        m_pPlayingTimer->stop();
    }

    emit stateChanged( state() );

    kdDebug() << "END " << k_funcinfo << endl;
}   // pause

// TODO depends on MAS support
void MasEngine::seek( unsigned int ms )
{
//    kdDebug() << "BEGIN " << k_funcinfo << endl;
    kdDebug() << " NOT IMPLEMENTED " << k_funcinfo << endl;
    kdDebug() << "  param: ms " << ms << endl;

//    kdDebug() << "END " << k_funcinfo << endl;
}   // seek


void MasEngine::setVolumeSW( unsigned int percent )
{
    kdDebug() << "BEGIN " << k_funcinfo << endl;
    kdDebug() << "  Param: percent " << percent << endl;

    m_volume = percent;

    //using a logarithmic function to make the volume slider more natural
    //m_volumeControl.scaleFactor( 1.0 - log10( ( 100 - percent) * 0.09 + 1.0 ) );
    //
    mas_mix_attenuate_db( m_mix_device, m_mix_sink, DB_CUTOFF*(10.0 - m_volume/10.0) );
    kdDebug() << "  after mas_mix_attenuate_db" << endl;

    kdDebug() << "END " << k_funcinfo << endl;
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
    int32 err;

    masc_log_verbosity( MAS_VERBLVL_DEBUG );

    masc_log_message( 0, (char*)"amarok/MASengine" );
    masc_log_message( 0, (char*)"tries to plays audio using MAS ;-)");
    masc_log_message( 0, (char*)"" );

    err = mas_init();
    kdDebug() << "  mas_init err:" << err << endl;
    if (err < 0)
    {
        kdWarning() << "masd not running.. trying to start" << endl;
        // masd seems not to be running, let's try to run it
        QCString cmdline;
        cmdline = QFile::encodeName( KStandardDirs::findExe( QString::fromLatin1( "kdesu" ) ) );
        // TODO !!!hardcoded path
        cmdline += " -n -f /usr/local/mas/log/mas-1.log -c ";
        cmdline += "/usr/local/mas/bin/mas-launch";

        kdDebug() << "  cmdline: " << cmdline << endl;
        int status = ::system( cmdline );
        kdDebug() << "  status: " << status << endl;
        ::sleep( 15 );

        if ( status != -1 && WIFEXITED( status ) )
        {
            // We could have a race-condition here.
            // The correct way to do it is to make artsd fork-and-exit
            // after starting to listen to connections (and running artsd
            // directly instead of using kdeinit), but this is better
            // than nothing.
            int time = 0;
            do
            {
                kdDebug() << "  time: " << time << endl;
                // every time it fails, we should wait a little longer
                // between tries
                ::sleep( 1 + time / 2 );
                err = mas_init();
                kdDebug() << "  mas_init err: " << err << endl;
            }
            while ( ++time < 5 && ( err < 0 ) );
        }
    }
    kdDebug() << "  mas_init err: " << err << endl;

    if (err < 0)
    {
        return false;
    }
    kdDebug() << "  mas_init successful ..." << endl;

    struct mas_data_characteristic* dc;
    struct mas_package nugget;
    mas_device_t anx;
    mas_port_t   tmp_source;
    mas_channel_t local;

    printf ("  before mas_get_local_control_channel\n");
    err = mas_get_local_control_channel( &local );
    if ( err < 0 ) masc_logerror( err, "getting local control channel" );
    printf ("  after mas_get_local_control_channel\n");

    /* Get the id of the sample clock provided by the anx device -- if
       we can! */
    printf ("  before mas_asm_get_device_by_name\n");
    err = mas_asm_get_device_by_name( (char*)"anx", &anx );
    mas_assert( err >= 0, (char*)"Couldn't get anx device" );
    printf ("  after mas_asm_get_device_by_name\n");

    printf ("  before mas_get (mc_clkid\n");
    m_sink_clkid = 0;
    err = mas_get( anx, (char*)"mc_clkid", NULL, &nugget );
    if ( err >= 0 )
    {
        masc_pull_int32( &nugget, &m_sink_clkid );
        masc_strike_package( &nugget );
    }
    printf ("  after mas_get (mc_clkid\n");

    /* CODEC */
    err = mas_asm_instantiate_device( (char*)"codec_mp1a_mad", 0, 0, &m_codec );
    if (err < 0 )
    {
        masc_log_message( MAS_VERBLVL_INFO, (char*)"Couldn't instantiate mp1a_mad codec, trying mp1a codec.");
        err = mas_asm_instantiate_device( (char*)"codec_mp1a", 0, 0, &m_codec );
        mas_assert( err >= 0, (char*)"Couldn't instantiate MPEG codec");
    }
    printf ("  after mas_asm_instantiate_device (codec_mp1a_mad)\n");

    /* source - instantiated on the local MAS server */
    err = mas_asm_instantiate_device_on_channel( (char*)"source_mp1a", 0, 0, &m_mp1a_source_device, local );
    mas_assert( err >= 0, (char*)"Couldn't instantiate source_mp1a" );

    /* buffer */
    err = mas_asm_instantiate_device( (char*)"sbuf", 0, 0, &m_sbuf );
    mas_assert( err >= 0, (char*)"Couldn't instantiate sbuf device" );
    printf ("  after mas_asm_instantiate_device (sbuf)\n");

    m_visual = NULL;

#ifdef USE_VISUAL
    err = mas_asm_instantiate_device( (char*)"visual", 0, 0, &m_visual );
    if ( err < 0 )
    {
        masc_log_message( 0, (char*)"Couldn't instantiate visual device.  It's okay, I just won't use it." );
        m_visual = NULL;
    }
    printf ("  after mas_asm_instantiate_device (visual)\n");
#endif

    /* id3 tag parser */
/* we have no need to read the tags from MAS */
/*
    err = mas_asm_instantiate_device_on_channel( (char*)"tag", 0, 0, &m_id3_device, local );
    mas_assert( err >= 0, (char*)"Couldn't instantiate tag device" );
*/
    /* get a handle to the mixer */
    err = mas_asm_get_device_by_name( (char*)"mix", &m_mix_device );
    mas_assert( err >= 0, (char*)"Couldn't get mixer device" );
    printf ("  after mas_asm_get_device_by_name (mix)\n");

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

    err = mas_asm_connect_devices_dc( m_codec, m_sbuf, (char*)"source", (char*)"sink", dc );
    mas_assert( err >= 0, "Couldn't connect MPEG codec to sbuf" );
    printf ("  after mas_asm_connect_device_dc (source, sink)\n");

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
    printf ("  after ... visual ...\n");

    /* set the buffer time */
    masc_setup_package( &nugget, NULL, 0, 0 );
    masc_pushk_uint32( &nugget, (char*)"buftime_ms", BUFFER_TIME_MS );
    masc_finalize_package( &nugget );
    mas_set( m_sbuf, (char*)"buftime_ms", &nugget );
    masc_strike_package( &nugget );

    printf ("  after ... masc_strike_package ...\n");

/*     masc_setup_package( &nugget, NULL, 0, 0 ); */
/*     masc_pushk_uint32( &nugget, "postout_time_ms", POSTOUT_TIME_MS ); */
/*     masc_finalize_package( &nugget ); */
/*     mas_set( m_sbuf, "postout_time_ms", &nugget ); */
/*     masc_strike_package( &nugget ); */

    /* If we can use a sample clock, let's do it... */
    if ( m_sink_clkid > 0 )
    {
        mas_get_mc_device( &m_sink_mc );
        mas_get_mc_device_on_channel( &m_source_mc, local );
    printf ("  after ... masc_get_mc_device_on_channel ...\n");

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

