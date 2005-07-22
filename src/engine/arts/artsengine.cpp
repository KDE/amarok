/***************************************************************************
                      artsengine.cpp  -  aRts audio interface
                         -------------------
begin                : Dec 31 2003
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

#define DEBUG_PREFIX "aRts-Engine"

#include "amarokarts.h"
// #include "artseffects.h"
#include "artsengine.h"
#include "debug.h"
#include "enginebase.h"

#include <math.h>            //setVolume(), timerEvent()
#include <string>
#include <vector>

#include <qdir.h>
#include <qdom.h>
#include <qfile.h>
#include <qlayout.h>
#include <qtextstream.h>
#include <qtimer.h>

#include <kapplication.h>
#include <kartswidget.h>
#include <kconfig.h>
#include <kfileitem.h>
#include <kgenericfactory.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetype.h>
#include <kstandarddirs.h>
#include <kurl.h>

#include <arts/artsflow.h>
#include <arts/artsgui.h>
#include <arts/artskde.h>
#include <arts/artsmodules.h>
#include <arts/connect.h>
#include <arts/dynamicrequest.h>
#include <arts/flowsystem.h>
#include <arts/kartsdispatcher.h>
#include <arts/kmedia2.h>
#include <arts/kplayobjectfactory.h>
#include <arts/soundserver.h>

#include <sys/wait.h>


AMAROK_EXPORT_PLUGIN( ArtsEngine )


ArtsEngine::ArtsEngine()
        : EngineBase()
        , m_artsDispatcher( new KArtsDispatcher( this ) )
        , m_playObject( 0 )
        , m_playObjectXfade( 0 )
        , m_scopeId( 0 )
        , m_volumeId( 0 )
        , m_xfadeFadeout( false )
        , m_xfadeValue( 0.0 )
        , m_xfadeCurrent( "invalue2" )
        , m_connectTimer( new QTimer( this ) )
{
    DEBUG_BLOCK

    addPluginProperty( "StreamingMode", "Socket" );
    addPluginProperty( "HasCrossfade",  "true" );
    addPluginProperty( "HasKIO",        "true" );
}


ArtsEngine::~ ArtsEngine()
{
    DEBUG_BLOCK

    m_connectTimer->stop();
    killTimers();
    delete m_playObject;
    delete m_playObjectXfade;
//     saveEffects();

    m_server            = Arts::SoundServerV2::null();
    m_scope             = Amarok::RawScope::null();
    m_xfade             = Amarok::Synth_STEREO_XFADE::null();
    m_volumeControl     = Arts::StereoVolumeControl::null();
    m_effectStack       = Arts::StereoEffectStack::null();
    m_globalEffectStack = Arts::StereoEffectStack::null();
    m_amanPlay          = Arts::Synth_AMAN_PLAY::null();
}


bool ArtsEngine::init()
{
    DEBUG_BLOCK

//     m_restoreEffects = restoreEffects;

    // We must restart artsd whenever we installed new mcopclasses
//     if ( restart )
//     {
//         QCString kill_cmdline;
//         kill_cmdline = "killall artsd";
//
//         int kill_status = ::system( kill_cmdline );
//         if ( kill_status != -1 && WIFEXITED( kill_status ) )
//         {
//             kdWarning() << "killall artsd succeeded." << endl;
//             restart = false;
//         }
//     }

    KConfig config( "kcmartsrc" );
    config.setGroup( "Arts" );

    const bool realtime = config.readBoolEntry( "StartRealtime", true );

    if ( !realtime )
        KMessageBox::information( 0, i18n(
            "<p>artsd is not running with <b>realtime priority</b> which may cause audio playback to \"skip\" and stutter.<p>"
            "<p>To use realtime priority, open the KDE Control Center and enable \"Run with highest possible priority\", under <i>Sound System</i> in the <i>Sound & Multimedia</i> branch. "
            "Some people may also have to check that \"$KDEDIR/bin/artswrapper\" is <b>set suid</b> (chmod +s).</p>"
            "<p>You may find, however, that playback is fine without increasing the priority of artsd.</p>" ),
            i18n( "aRts Problem" ), "artsRealtimeAdvice" );

    m_server = Arts::Reference( "global:Arts_SoundServerV2" );

    if ( m_server.isNull() || m_server.error() )
    {
        warning() << "aRtsd not running.. trying to start" << endl;
        // aRts seems not to be running, let's try to run it
        // First, let's read the configuration as in kcmarts
        QCString cmdline;

        bool x11Comm = config.readBoolEntry( "X11GlobalComm", false );

        // put the value of x11Comm into .mcoprc
        {
            KConfig X11CommConfig( QDir::homeDirPath() + "/.mcoprc" );

            if ( x11Comm )
                X11CommConfig.writeEntry( "GlobalComm", "Arts::X11GlobalComm" );
            else
                X11CommConfig.writeEntry( "GlobalComm", "Arts::TmpGlobalComm" );

            X11CommConfig.sync();
        }

        cmdline = QFile::encodeName( KStandardDirs::findExe( QString::fromLatin1( "kdeinit_wrapper" ) ) );
        cmdline += " ";

        if ( realtime )
            cmdline += QFile::encodeName( KStandardDirs::findExe(
                                              QString::fromLatin1( "artswrapper" ) ) );
        else
            cmdline += QFile::encodeName( KStandardDirs::findExe(
                                              QString::fromLatin1( "artsd" ) ) );

        cmdline += " ";
        cmdline += config.readEntry( "Arguments", "-F 10 -S 4096 -s 60 -m artsmessage -l 3 -f -n" ).utf8();

        int status = ::system( cmdline );

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
                // every time it fails, we should wait a little longer
                // between tries
                ::sleep( 1 + time / 2 );
                m_server = Arts::Reference( "global:Arts_SoundServerV2" );
            }
            while ( ++time < 5 && ( m_server.isNull() ) );
        }
    }

    if ( m_server.isNull() )
    {
        KMessageBox::error( 0, i18n( "Cannot start aRts. You must use another engine." ), i18n( "Fatal Error" ) );
        return false;
    }

    { //amanPlay
        m_amanPlay = Arts::DynamicCast( m_server.createObject( "Arts::Synth_AMAN_PLAY" ) );
        m_amanPlay.title( "amarok" );
        m_amanPlay.autoRestoreID( "amarok" );
        m_amanPlay.start();
    }

    { //Xfade
        m_xfade = Arts::DynamicCast( m_server.createObject( "Amarok::Synth_STEREO_XFADE" ) );

        if ( m_xfade.isNull() ) {
            KMessageBox::error( 0,
                                i18n( "<p>There was an error loading libamarokarts. First try:"
                                      "<pre>killall -9 artsd && amarok</pre>"
                                      "If that does not work then amaroK was probably installed with the wrong prefix; "
                                      "please re-configure amaroK using:"
                                      "<pre>./configure --prefix=`kde-config --prefix` && su -c \"make install\"</pre>" ),
                                i18n( "amaroK Error" ) );
            return false;
        }

        m_xfade.percentage( m_xfadeValue );
        m_xfade.start();
    }

    { //globalEffectStack
        m_globalEffectStack = Arts::DynamicCast( m_server.createObject( "Arts::StereoEffectStack" ) );
        m_globalEffectStack.start();
    }

    { //effectStack
        m_effectStack = Arts::DynamicCast( m_server.createObject( "Arts::StereoEffectStack" ) );
        m_effectStack.start();
        m_globalEffectStack.insertBottom( m_effectStack, "Effect Stack" );
    }

    { //scope
        m_scope = Arts::DynamicCast( m_server.createObject( "Amarok::RawScope" ) );
        m_scope.start();
        m_scope.buffer( SCOPE_SIZE );
        m_scopeId = m_globalEffectStack.insertTop( m_scope, "Analyzer" );
    }

    Arts::connect( m_globalEffectStack  , m_amanPlay );
    Arts::connect( m_xfade, "outvalue_l", m_globalEffectStack, "inleft" );
    Arts::connect( m_xfade, "outvalue_r", m_globalEffectStack, "inright" );

    { //volume mixer
        m_volumeControl = Arts::DynamicCast( m_server.createObject( "Arts::StereoVolumeControl" ) );
        m_volumeControl.start();
        m_volumeId = m_globalEffectStack.insertBottom( m_volumeControl, "Volume Control" );
    }

//     if ( m_restoreEffects ) loadEffects();
    startTimer( ARTS_TIMER );
    connect( m_connectTimer, SIGNAL( timeout() ), this, SLOT( connectTimeout() ) );

    return true;
}


////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
////////////////////////////////////////////////////////////////////////////////

bool ArtsEngine::canDecode( const KURL &url ) const
{
    KFileItem fileItem( KFileItem::Unknown, KFileItem::Unknown, url, false ); //false == determineMimeType straight away
    KMimeType::Ptr mimetype = fileItem.determineMimeType();

    Arts::TraderQuery query;
    query.supports( "Interface", "Arts::PlayObject" );
    query.supports( "MimeType", mimetype->name().latin1() );
    std::vector<Arts::TraderOffer> *offers = query.query();

    bool result = !offers->empty();
    delete offers;

    return result;
}


uint ArtsEngine::position() const
{
    if ( m_playObject ) {
        const Arts::poTime time = m_playObject->currentTime();
        return time.seconds * 1000 + time.ms;
    }

    return 0;
}


uint ArtsEngine::length() const
{
    if ( m_playObject ) {
        const Arts::poTime time = m_playObject->overallTime();
        return time.seconds * 1000 + time.ms;
    }

    return 0;
}


Engine::State ArtsEngine::state() const
{
    if ( m_playObject && !m_playObject->isNull() )
    {
        if ( m_playObject->object().isNull() )
            return Engine::Playing;

        switch ( m_playObject->state() )
        {
            case Arts::posPaused:
                return Engine::Paused;
            case Arts::posPlaying:
                return Engine::Playing;
            case Arts::posIdle:
                return Engine::Idle;
        }
    }

    return Engine::Empty;
}


const Engine::Scope& ArtsEngine::scope()
{
    static Engine::Scope out;
    out.resize( SCOPE_SIZE );
    std::vector<float>* in = m_scope.scope();

    if ( in && in->size() >= SCOPE_SIZE ) {
        // Convert float to int
        for ( uint i = 0; i < SCOPE_SIZE; i++ )
            out[i] = (int16_t) ( ( *in )[i] * (float) ( 1<<14 ) );

        delete in;
    }

    return out;
}

//////////////////////////////////////////////////////////////////////

bool ArtsEngine::load( const KURL& url, bool stream )
{
    DEBUG_BLOCK

    Engine::Base::load( url, stream );
    debug() << "Loading url: " << url.url() << endl;

    debug() << "url.path()     == " << url.path()     << endl;
    debug() << "url.protocol() == " << url.protocol() << endl;
    debug() << "url.host()     == " << url.host()     << endl;
    debug() << "url.port()     == " << url.port()     << endl;

    m_xfadeFadeout = false;
    startXfade();

    KDE::PlayObjectFactory factory( m_server );
    m_playObject = factory.createPlayObject( url, false ); //second parameter: create BUS(true/false)

    if ( !m_playObject || m_playObject->isNull() ) {
        connectTimeout();
    }
    else
    {
//         connect( m_playObject, SIGNAL( destroyed() ), this, SIGNAL( stopped() ) );

        if ( m_playObject->object().isNull() ) {
            debug() << "m_playObject->object().isNull()" << endl;

            connect( m_playObject, SIGNAL( playObjectCreated() ), this, SLOT( connectPlayObject() ) );
            m_connectTimer->start( TIMEOUT, true );
        }
        else {
            connectPlayObject();
        }

        play();
    }

    return true;
}


void ArtsEngine::connectPlayObject() //SLOT
{
    m_connectTimer->stop();

    if ( m_playObject && !m_playObject->isNull() && !m_playObject->object().isNull() )
    {
        m_playObject->object()._node()->start();

        //switch xfade channels
        m_xfadeCurrent = ( m_xfadeCurrent == "invalue1" ) ? "invalue2" : "invalue1";

        if ( m_xfadeValue == 0.0 )
            m_xfadeValue = 1.0;

        Arts::connect( m_playObject->object(), "left", m_xfade, ( m_xfadeCurrent + "_l" ).latin1() );
        Arts::connect( m_playObject->object(), "right", m_xfade, ( m_xfadeCurrent + "_r" ).latin1() );
    }
}

//SLOT
void ArtsEngine::connectTimeout()
{
    error() << "Cannot initialize PlayObject! Skipping this track." << endl;
    m_connectTimer->stop();

    delete m_playObject;
    m_playObject = NULL;
}


bool ArtsEngine::play( uint offset )
{
    if ( !m_playObject ) return false;

    m_playObject->play();

    // Seek to right position when using "Resume playback at startup"
    if ( offset )
        seek( offset );

    emit stateChanged( Engine::Playing );

    return true;
}


void ArtsEngine::stop()
{
    DEBUG_BLOCK

    //switch xfade channels
    m_xfadeCurrent = ( m_xfadeCurrent == "invalue1" ) ? "invalue2" : "invalue1";

    if ( m_xfadeValue == 0.0 )
        m_xfadeValue = 1.0;

    m_xfadeFadeout = true;
    startXfade();

    emit stateChanged( Engine::Empty );
}


void ArtsEngine::pause()
{
    if ( !m_playObject ) return;

    if ( state() == Engine::Paused )
        m_playObject->play();
    else
        m_playObject->pause();

    emit stateChanged( state() );
}


void ArtsEngine::seek( uint ms )
{
    if ( m_playObject )
    {
        Arts::poTime time;
        time.ms      = ms % 1000;
        time.seconds = ( ms - time.ms ) / 1000;
        time.custom  = 0.0;

        m_playObject->seek( time );
    }
}


bool ArtsEngine::decoderConfigurable()
{
/*    if ( m_playObject && !m_playObject->object().isNull() && !m_pDecoderConfigWidget )
    {
        Arts::TraderQuery query;
        query.supports( "Interface", "Arts::GuiFactory" );
        query.supports( "CanCreate", m_playObject->object()._interfaceName() );

        std::vector<Arts::TraderOffer> *queryResults = query.query();
        bool yes = queryResults->size();
        delete queryResults;

        return yes;
    }*/
    return false;
}


void ArtsEngine::configureDecoder() //slot
{
/*    //this method shows a GUI for an aRts CODEC. currently only working with markey's modplug_artsplugin

    if ( m_playObject && !m_pDecoderConfigWidget )
    {
        m_pDecoderConfigWidget = new ArtsConfigWidget( m_playObject->object() );
        connect( m_playObject, SIGNAL( destroyed() ), m_pDecoderConfigWidget, SLOT( deleteLater() ) );

        m_pDecoderConfigWidget->show();
    }*/
}


////////////////////////////////////////////////////////////////////////////////
// PROTECTED
////////////////////////////////////////////////////////////////////////////////

void ArtsEngine::setVolumeSW( uint percent )
{
    if ( m_volumeId ) {
        //using a logarithmic function to make the volume slider more natural
        m_volumeControl.scaleFactor( 1.0 - log10( ( 100 - percent) * 0.09 + 1.0 ) );
    }
}


////////////////////////////////////////////////////////////////////////////////
// PRIVATE
////////////////////////////////////////////////////////////////////////////////

void ArtsEngine::startXfade()
{
     if ( m_playObjectXfade )
    {
        m_playObjectXfade->halt();
        delete m_playObjectXfade;
    }

    m_playObjectXfade = m_playObject;
    m_playObject = 0;
}


void ArtsEngine::timerEvent( QTimerEvent* )
{
   if( state() == Engine::Idle )
       emit trackEnded();

   if ( m_xfadeValue > 0.0 )
   {
        m_xfadeValue -= ( m_xfadeLength ) ?  1.0 / m_xfadeLength * ARTS_TIMER : 1.0;

        if ( m_xfadeValue <= 0.0 )
        {
            m_xfadeValue = 0.0;
            if ( m_playObjectXfade )
            {
                m_playObjectXfade->halt();
                delete m_playObjectXfade;
                m_playObjectXfade = 0;
            }
        }
        float value;
        if ( m_xfadeFadeout )
            value = 1.0 - log10( ( 1.0 - m_xfadeValue ) * 9.0 + 1.0 );
        else
            value = log10( m_xfadeValue * 9.0 + 1.0 );

        m_xfade.percentage( ( m_xfadeCurrent == "invalue2" ) ? value : 1.0 - value );
//         debug() << k_funcinfo << "percentage: " << m_xfade.percentage() << endl;
    }
}


#include "artsengine.moc"
