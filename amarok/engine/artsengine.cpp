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

#include "amarokarts.h"
#include "artsengine.h"

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
#include <kartswidget.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kfileitem.h>
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


ArtsEngine::ArtsEngine( bool& restart, int scopeSize )
        : EngineBase()
        , m_pPlayObject( 0 )
        , m_pPlayObjectXfade( 0 )
        , m_scopeId( 0 )
        , m_scopeSize( 1 << scopeSize )
        , m_volumeId( 0 )
        , m_xfadeFadeout( false )
        , m_xfadeValue( 0.0 )
        , m_xfadeCurrent( "invalue2" )
{
    setName( "arts" );  //name is used for RTTI
    m_mixerHW = -1;     //initialize

    // We must restart artsd whenever we installed new mcopclasses
    if ( restart )
    {
        QCString kill_cmdline;
        kill_cmdline = "killall artsd";

        int kill_status = ::system( kill_cmdline );
        if ( kill_status != -1 && WIFEXITED( kill_status ) )
        {
            kdDebug() << "killall artsd succeeded." << endl;
            restart = false;
        }
    }

    KConfig config( "kcmartsrc" );
    config.setGroup( "Arts" );

    bool realtime = config.readBoolEntry( "StartRealtime", true );

    if ( !realtime )
        KMessageBox::information( 0, i18n( "<p>artsd is not running with <b>realtime priority</b> which may cause audio playback to \"skip\" and stutter.<p>"
                                     "<p>To use realtime priority, open the KDE Control Center and enable \"Run with highest possible priority\", under <i>Sound System</i> in the <i>Sound & Multimedia</i> branch. "
                                     "Some people may also have to check that \"$KDEDIR/bin/artswrapper\" is <b>set suid</b> (chmod +s).</p>"
                                     "<p>You may find, however, that playback is fine without increasing the priority of artsd.</p>" ),
                               i18n( "aRts Problem" ), "artsRealtimeAdvice" );

    m_pArtsDispatcher = new KArtsDispatcher();
    m_server = Arts::Reference( "global:Arts_SoundServerV2" );

    if ( m_server.isNull() || m_server.error() )
    {
        kdDebug() << "aRtsd not running.. trying to start" << endl;
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
        KMessageBox::error( 0, i18n( "Cannot start aRts. Exiting." ), i18n( "Fatal Error" ) );
        ::exit( 1 );
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
                                i18n( "Cannot find libamarokarts. Probably amaroK was installed with the "
                                      "wrong prefix. Please install again using: ./configure "
                                      "--prefix=`kde-config --prefix`" ),
                                i18n( "Fatal Error" ) );
            ::exit( 1 );
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
        m_scope.buffer( m_scopeSize );
        m_scopeId = m_globalEffectStack.insertTop( m_scope, "Analyzer" );
    }

    Arts::connect( m_globalEffectStack  , m_amanPlay );
    Arts::connect( m_xfade, "outvalue_l", m_globalEffectStack, "inleft" );
    Arts::connect( m_xfade, "outvalue_r", m_globalEffectStack, "inright" );

    if ( m_restoreEffects ) loadEffects();
    startTimer( ARTS_TIMER );
}


ArtsEngine::~ ArtsEngine()
{
    emit endOfTrack();
    delete m_pPlayObject;
    delete m_pPlayObjectXfade;
    saveEffects();

    m_scope             = Amarok::RawScope::null();
    m_xfade             = Amarok::Synth_STEREO_XFADE::null();
    m_volumeControl     = Arts::StereoVolumeControl::null();
    m_effectStack       = Arts::StereoEffectStack::null();
    m_globalEffectStack = Arts::StereoEffectStack::null();
    m_amanPlay          = Arts::Synth_AMAN_PLAY::null();
    m_server            = Arts::SoundServerV2::null();
}


bool ArtsEngine::initMixer( bool hardware )
{
    { //make sure any previously started volume control gets killed
        if ( m_volumeId )
        {
            m_globalEffectStack.remove( m_volumeId );
            m_volumeId = 0;
            m_volumeControl = Arts::StereoVolumeControl::null();
        }
        closeMixerHW();
    }

    if ( hardware )
        hardware = initMixerHW();
    else
    {
        m_volumeControl = Arts::DynamicCast( m_server.createObject( "Arts::StereoVolumeControl" ) );
        m_volumeControl.start();
        m_volumeId = m_globalEffectStack.insertBottom( m_volumeControl, "Volume Control" );
    }

    setVolume( m_volume );
    return hardware;
}

////////////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
////////////////////////////////////////////////////////////////////////////////

bool ArtsEngine::canDecode( const KURL &url, mode_t mode, mode_t permissions )
{
    KFileItem fileItem( mode, permissions, url, false ); //false = determineMimeType straight away
    KMimeType::Ptr mimetype = fileItem.determineMimeType();

    Arts::TraderQuery query;
    query.supports( "Interface", "Arts::PlayObject" );
    query.supports( "MimeType", mimetype->name().latin1() );
    std::vector<Arts::TraderOffer> *offers = query.query();

    bool result = !offers->empty();
    delete offers;

    return result;
}


long ArtsEngine::position() const
{
    if ( m_pPlayObject )
        return m_pPlayObject->currentTime().seconds * 1000 +
               m_pPlayObject->currentTime().ms;
    else
        return 0;
}


EngineBase::EngineState ArtsEngine::state() const
{
    if ( m_pPlayObject && !m_pPlayObject->isNull() )
    {
        switch ( m_pPlayObject->state() )
        {
            case Arts::posPaused:
                return Paused;
            case Arts::posPlaying:
                return Playing;
            case Arts::posIdle:
                return Idle;
        }
    }
    else
        return EngineBase::Empty;
}


bool ArtsEngine::isStream() const
{
    if ( m_pPlayObject )
        return m_pPlayObject->stream();
    else
        return false;
}


std::vector<float>* ArtsEngine::scope()
{
    return m_scope.scope();
}

//////////////////////////////////////////////////////////////////////

void ArtsEngine::open( const KURL& url )
{
    m_xfadeFadeout = false;
    startXfade();

    KDE::PlayObjectFactory factory( m_server );
    m_pPlayObject = factory.createPlayObject( url, false ); //second parameter: create BUS(true/false)

    if ( !m_pPlayObject || m_pPlayObject->isNull() )
        connectTimeout();
    else
    {
        if ( m_pPlayObject->object().isNull() )
        {            
            kdDebug() << "[void ArtsEngine::open()] m_pPlayObject->object().isNull()" << endl;
            connect( m_pPlayObject, SIGNAL( playObjectCreated() ), this, SLOT( connectPlayObject() ) );
            QTimer::singleShot( TIMEOUT, this, SLOT( connectTimeout() ) );
        }
        else
            connectPlayObject();
    }
}


void ArtsEngine::connectPlayObject()
{
    kdDebug() << "[void ArtsEngine::connectPlayObject()]" << endl;
    
    if ( !m_pPlayObject->object().isNull() )
    {
        m_pPlayObject->object()._node()->start();

        //switch xfade channels
        m_xfadeCurrent = ( m_xfadeCurrent == "invalue1" ) ? "invalue2" : "invalue1";
    
        if ( m_xfadeValue == 0.0 )
            m_xfadeValue = 1.0;
        
        Arts::connect( m_pPlayObject->object(), "left", m_xfade, ( m_xfadeCurrent + "_l" ).latin1() );
        Arts::connect( m_pPlayObject->object(), "right", m_xfade, ( m_xfadeCurrent + "_r" ).latin1() );
    
    }
    else
        //something went wrong, we can't play this track
        connectTimeout();
}

//SLOT
void ArtsEngine::connectTimeout()
{        
    if ( m_pPlayObject && !m_pPlayObject->isNull() && m_pPlayObject->object().isNull() )
    {
        kdWarning() << "[ArtsEngine::open()] Cannot initialize PlayObject! Skipping this track." << endl;
        
        emit endOfTrack();
        delete m_pPlayObject;
        m_pPlayObject = NULL;
    }
}
        

void ArtsEngine::play()
{
    if ( m_pPlayObject )
    {
        m_pPlayObject->play();
    }
}


void ArtsEngine::stop()
{
    kdDebug() << "[void ArtsEngine::stop()]" << endl;

    emit endOfTrack();
    //switch xfade channels
    m_xfadeCurrent = ( m_xfadeCurrent == "invalue1" ) ? "invalue2" : "invalue1";

    if ( m_xfadeValue == 0.0 )
        m_xfadeValue = 1.0;
   
    m_xfadeFadeout = true;
    startXfade();
}


void ArtsEngine::pause()
{
    if ( m_pPlayObject )
    {
        m_pPlayObject->pause();
    }
}


void ArtsEngine::seek( long ms )
{
    if ( m_pPlayObject )
    {
        Arts::poTime time;
        time.ms      = ms % 1000;
        time.seconds = ( ms - time.ms ) / 1000;
        time.custom  = 0.0;

        m_pPlayObject->seek( time );
    }
}


void ArtsEngine::setVolume( int percent )
{
    m_volume = percent;

    if ( m_volumeId )
        m_volumeControl.scaleFactor( percent * 0.01 );
    else
        EngineBase::setVolumeHW( percent );
}


QStringList ArtsEngine::availableEffects() const
{
    QStringList val;
    Arts::TraderQuery query;
    query.supports( "Interface", "Arts::StereoEffect" );
    query.supports( "Interface", "Arts::SynthModule" );
    std::vector<Arts::TraderOffer> *offers = query.query();

    for ( std::vector<Arts::TraderOffer>::iterator i = offers->begin(); i != offers->end(); i++ )
    {
        Arts::TraderOffer &offer = *i;
        QCString name = offer.interfaceName().c_str();
        val.append( name );
    }
    delete offers;

    return val;
}


std::vector<long> ArtsEngine::activeEffects() const
{
    std::vector<long> vec;
    QMap<long, EffectContainer>::ConstIterator it;

    for ( it = m_effectMap.begin(); it != m_effectMap.end(); ++it )
    {
        vec.push_back( it.key() );
    }

    return vec;
}


QString ArtsEngine::effectNameForId( long id ) const
{
    const std::string str = (*m_effectMap[id].effect)._interfaceName();
    QString qstr( str.c_str() );

    return qstr;
}


bool ArtsEngine::effectConfigurable( long id ) const
{
    if ( m_effectMap[id].widget )
        return false;

    Arts::TraderQuery query;
    query.supports( "Interface", "Arts::GuiFactory" );
    query.supports( "CanCreate", (*m_effectMap[id].effect)._interfaceName() );

    std::vector<Arts::TraderOffer> *queryResults = query.query();
    bool yes = queryResults->size();
    delete queryResults;

    return yes;
}


long ArtsEngine::createEffect( const QString& name )
{
    Arts::StereoEffect* pFX = new Arts::StereoEffect;
    *pFX = Arts::DynamicCast( m_server.createObject( std::string( name.ascii() ) ) );

    if ( (*pFX).isNull() ) {
        kdWarning() << "[ArtsEngine::createEffect] error: could not create effect." << endl;
        delete pFX;
        return 0;
    }

    pFX->start();
    long id = m_effectStack.insertBottom( *pFX, std::string( name.ascii() ) );

    if ( !id ) {
        kdWarning() << "[ArtsEngine::createEffect] error: insertBottom failed." << endl;
        pFX->stop();
        delete pFX;
        return 0;
    }

    EffectContainer container;
    container.effect = pFX;
    container.widget = 0;

    m_effectMap.insert( id, container );

    return id;
}


void ArtsEngine::removeEffect( long id )
{
    m_effectStack.remove( id );

    m_effectMap[id].effect->stop();
    delete m_effectMap[id].widget;
    delete m_effectMap[id].effect;

    m_effectMap.remove( id );
}


void ArtsEngine::configureEffect( long id )
{
    m_effectMap[id].widget = new ArtsConfigWidget( *m_effectMap[id].effect );
    m_effectMap[id].widget->show();

    m_effectMap[id].widget->m_pPointer = &m_effectMap[id].widget;
}

////////////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
////////////////////////////////////////////////////////////////////////////////

void ArtsEngine::startXfade()
{
     if ( m_pPlayObjectXfade )
    {
        m_pPlayObjectXfade->halt();
        delete m_pPlayObjectXfade;
        emit endOfTrack();
    }

    m_pPlayObjectXfade = m_pPlayObject;
    m_pPlayObject = 0;
}


#include <math.h>
void ArtsEngine::timerEvent( QTimerEvent* )
{
    if ( m_xfadeValue > 0.0 )
    {
        m_xfadeValue -= ( m_xfadeLength ) ?  1.0 / m_xfadeLength * ARTS_TIMER : 1.0;

        if ( m_xfadeValue <= 0.0 )
        {
            m_xfadeValue = 0.0;
            if ( m_pPlayObjectXfade )
            {
                m_pPlayObjectXfade->halt();
                delete m_pPlayObjectXfade;
                m_pPlayObjectXfade = 0;
                emit endOfTrack();
            }
        }
        float value;
        if ( m_xfadeFadeout )
            value = 1.0 - log10( ( 1.0 - m_xfadeValue ) * 9.0 + 1.0 );
        else
            value = log10( m_xfadeValue * 9.0 + 1.0 );

        m_xfade.percentage( ( m_xfadeCurrent == "invalue2" ) ? value : 1.0 - value );
        kdDebug() << "[timerEvent] percentage: " << m_xfade.percentage() << endl;
    }
}


void ArtsEngine::loadEffects()
{
    kdDebug() << "[ArtsEngine::loadEffects()]" << endl;

    QDomDocument doc;
    QFile file( kapp->dirs()->saveLocation( "data", kapp->instanceName() + "/" ) + "arts-effects.xml" );

    if ( !file.open( IO_ReadOnly ) )
    {
        kdWarning() << "[ArtsEngine::loadEffects()] error: !file.open()" << endl;
        return;
    }

    QString errorMsg;
    int     errorLine;
    int     errorColumn;
    if ( !doc.setContent( &file, &errorMsg, &errorLine, &errorColumn ) )
    {
        kdWarning() << "[ArtsEngine::loadEffects()] error: !doc.setContent()" << endl;
        kdWarning() << "[ArtsEngine::loadEffects()] errorMsg   : " << errorMsg    << endl;
        kdWarning() << "[ArtsEngine::loadEffects()] errorLine  : " << errorLine   << endl;
        kdWarning() << "[ArtsEngine::loadEffects()] errorColumn: " << errorColumn << endl;
        file.close();
        return;
    }

    QDomElement docElem = doc.documentElement();

    for ( QDomNode n = docElem.firstChild(); !n.isNull(); n = n.nextSibling() )
    {
        QString effect = n.namedItem( "effectname" ).firstChild().toText().nodeValue();
        kdDebug() << "effectname: " << effect << endl;

        long id = createEffect( effect );

        for ( QDomNode nAttr = n.firstChild(); id && !nAttr.isNull(); nAttr = nAttr.nextSibling() )
        {
            if ( nAttr.nodeName() == "attribute" )
            {
                QString name  = nAttr.namedItem( "name"  ).firstChild().toText().nodeValue();
                QString type  = nAttr.namedItem( "type"  ).firstChild().toText().nodeValue();
                QString value = nAttr.namedItem( "value" ).firstChild().toText().nodeValue();

                kdDebug() << "name : " << name  << endl;
                kdDebug() << "type : " << type  << endl;
                kdDebug() << "value: " << value << endl;

                Arts::DynamicRequest req( *m_effectMap[id].effect );
                std::string set( "_set_" );
                set.append( std::string( name.latin1() ) );
                req.method( set );

                Arts::Buffer buf;
                buf.fromString( std::string( value.latin1() ), "" );

                Arts::Any param;
                param.type = std::string( type.latin1() );
                param.readType( buf );
                req.param( param );

                if ( !req.invoke() )
                    kdWarning() << "DynamicRequest failed." << endl;
            }
        }
    }
}


void ArtsEngine::saveEffects()
{
    QDomDocument doc;
    QDomElement root = doc.createElement( "aRts-Effects" );
    doc.appendChild( root );

    for ( QMap<long, EffectContainer>::Iterator it = m_effectMap.begin(); it != m_effectMap.end(); ++it )
    {
        QDomElement tagEffect = doc.createElement( "effect" );
        root.appendChild( tagEffect );

            { //effectname
                QDomElement tag = doc.createElement( "effectname" );
                tagEffect.appendChild( tag );
                QDomText txt = doc.createTextNode( (*it.data().effect)._interfaceName().c_str() );
                tag.appendChild( txt );
            }

        Arts::InterfaceDef def = (*it.data().effect)._queryInterface( (*it.data().effect)._interfaceName() );

        for ( uint i = 0; i < def.attributes.size(); i++ )
        {
            QDomElement tagAttribute = doc.createElement( "attribute" );
            tagEffect.appendChild( tagAttribute );

            { //name
                QDomElement tag = doc.createElement( "name" );
                tagAttribute.appendChild( tag );
                QDomText txt = doc.createTextNode( def.attributes[i].name.c_str() );
                tag.appendChild( txt );
            }

            Arts::DynamicRequest req( *it.data().effect );
            req.method( "_get_" + def.attributes[i].name );
            Arts::Any result;
            result.type = def.attributes[i].type;

            { //type
                QDomElement tag = doc.createElement( "type" );
                tagAttribute.appendChild( tag );
                QDomText txt = doc.createTextNode( def.attributes[i].type.c_str() );
                tag.appendChild( txt );
            }

            if ( !req.invoke( result ) )
                kdWarning() << "request failed." << endl;

            Arts::Buffer buf;
            result.writeType( buf );

            { //value
                QDomElement tag = doc.createElement( "value" );
                tagAttribute.appendChild( tag );
                QDomText txt = doc.createTextNode( buf.toString( "" ).c_str() );
                tag.appendChild( txt );
            }
        }
        removeEffect( it.key() );
    }

    QString path = kapp->dirs()->saveLocation( "data", kapp->instanceName() + "/" ) + "arts-effects.xml";
    QFile::remove( path );
    QFile file( path );
    file.open( IO_ReadWrite );
    QTextStream stream( &file );
    stream << doc;
}


// CLASS EffectConfigWidget --------------------------------------------------------

ArtsEngine::ArtsConfigWidget::ArtsConfigWidget( Arts::Object object )
        : QWidget( 0, 0, Qt::WType_TopLevel | Qt::WDestructiveClose )
{
    setCaption( kapp->makeStdCaption( QString( object._interfaceName().c_str() ) ) );

    Arts::GenericGuiFactory factory;
    m_gui = factory.createGui( object );

    if ( m_gui.isNull() )
    {
        kdWarning() << "Arts::Widget gui == NULL! Returning.." << endl;
        return;
    }

    else
    {
        m_pArtsWidget = new KArtsWidget( m_gui, this );

        QBoxLayout *lay = new QHBoxLayout( this );
        lay->add( m_pArtsWidget );
    }
}


ArtsEngine::ArtsConfigWidget::~ArtsConfigWidget()
{
    delete m_pArtsWidget;
    m_gui = Arts::Widget::null();
    *m_pPointer = 0;
}


#include "artsengine.moc"
