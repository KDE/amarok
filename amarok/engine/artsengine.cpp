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

#include "../amarokarts/amarokarts.h"
#include "artsengine.h"
#include "enginebase.h"
#include "../titleproxy/titleproxy.h"

#include <string>
#include <vector>

#include <qdir.h>
#include <qdom.h>
#include <qfile.h>
#include <qlayout.h>
#include <qtimer.h>
#include <qstring.h>
#include <qtextstream.h>

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


const int ArtsEngine::ARTS_TIMER = 100;


ArtsEngine::ArtsEngine( bool& restart, int scopeSize )
        : EngineBase()
        , m_pPlayObject( 0 )
        , m_pPlayObjectXfade( 0 )
        , m_scopeId( 0 )
        , m_scopeSize( 1 << scopeSize )
        , m_volumeId( 0 )
        , m_proxyError( false )
        , m_xfadeFadeout( false )
        , m_xfadeValue( 0.0 )
        , m_xfadeCurrent( "invalue1" )
{
    setName( "arts" );
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

    m_pArtsDispatcher = new KArtsDispatcher();

    m_server = Arts::Reference( "global:Arts_SoundServerV2" );
    if ( m_server.isNull() || m_server.error() )
    {
        kdDebug() << "aRtsd not running.. trying to start" << endl;
        // aRts seems not to be running, let's try to run it
        // First, let's read the configuration as in kcmarts
        KConfig config( "kcmartsrc" );
        QCString cmdline;

        config.setGroup( "Arts" );

        bool rt = config.readBoolEntry( "StartRealtime", false );
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

        if ( rt )
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
        KMessageBox::error( 0, i18n( "Cannot start aRts! Exiting." ), i18n( "Fatal Error" ) );
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
                                i18n( "Cannot find libamarokarts! Probably amaroK was installed with the \
                                    wrong prefix. Please install again using: ./configure \
                                    --prefix=`kde-config --prefix`" ),
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
        enableScope();
    }

    Arts::connect( m_globalEffectStack  , m_amanPlay );
    Arts::connect( m_xfade, "outvalue_l", m_globalEffectStack, "inleft" );
    Arts::connect( m_xfade, "outvalue_r", m_globalEffectStack, "inright" );
            
    loadEffects();
    startTimer( ARTS_TIMER );
}


ArtsEngine::~ ArtsEngine()
{
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
    if ( m_pPlayObject )
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

void ArtsEngine::open( KURL url )
{
    if ( !m_proxyError )
    {
        m_xfadeFadeout = false;
        startXfade();
    }
        
    KDE::PlayObjectFactory factory( m_server );

    if ( /* m_optTitleStream && */ !m_proxyError && !url.isLocalFile()  )
    {
        TitleProxy *pProxy = new TitleProxy( url );
        m_pPlayObject = factory.createPlayObject( pProxy->proxyUrl(), false );

        connect( pProxy, SIGNAL( metaData         ( QString, QString, QString ) ),
                 this,   SLOT  ( receiveStreamMeta( QString, QString, QString ) ) );
        connect( pProxy, SIGNAL( error() ), this, SLOT( proxyError() ) );
        connect( m_pPlayObject, SIGNAL( destroyed() ), pProxy, SLOT( deleteLater() ) );
    }
    else
        m_pPlayObject = factory.createPlayObject( url, false ); //second parameter: create BUS(true/false)

    m_proxyError = false;

    if ( !m_pPlayObject  )
    {
        kdDebug() << "Can't initialize Playobject. m_pPlayObject == NULL." << endl;
        return;
    }
    if ( m_pPlayObject->isNull() )
    {
        kdDebug() << "Can't initialize Playobject. m_pPlayObject->isNull()." << endl;
        delete m_pPlayObject;
        m_pPlayObject = NULL;
        return;
    }

    if ( m_pPlayObject->object().isNull() )
        connect( m_pPlayObject, SIGNAL( playObjectCreated() ), this, SLOT( connectPlayObject() ) );
    else
        connectPlayObject();

    return;
}


void ArtsEngine::connectPlayObject()
{
    if ( !m_pPlayObject->object().isNull() )
    {
        m_pPlayObject->object()._node()->start();

        Arts::connect( m_pPlayObject->object(), "left", m_xfade, ( m_xfadeCurrent + "_l" ).latin1() );
        Arts::connect( m_pPlayObject->object(), "right", m_xfade, ( m_xfadeCurrent + "_r" ).latin1() );
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
    pFX->start();

    long id = m_effectStack.insertBottom( *pFX, std::string( name.ascii() ) );
    
    if ( !id )
    {
        kdDebug() << "insertBottom failed" << endl;
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

void ArtsEngine::disableScope()
{
    if ( m_scopeId )
    {
        m_scope.stop();
        m_globalEffectStack.remove( m_scopeId );
        m_scopeId = 0;
    }
}


void ArtsEngine::enableScope()
{
    if ( !m_scopeId )
    {
        m_scope.start();
        m_scope.buffer( m_scopeSize );
        m_scopeId = m_globalEffectStack.insertTop( m_scope, "Analyzer" );
    }
}


void ArtsEngine::proxyError()
{
    m_proxyError = true;
    play();
}


void ArtsEngine::receiveStreamMeta( QString title, QString url, QString kbps )
{
    emit metaData( title, url, kbps );
}


void ArtsEngine::startXfade()
{
    //switch xfade channels
    m_xfadeCurrent = ( m_xfadeCurrent == "invalue1" ) ? "invalue2" : "invalue1";

    if ( m_xfadeValue == 0.0 )
        m_xfadeValue = 1.0;
    
    if ( m_pPlayObjectXfade )
    {
        m_pPlayObjectXfade->halt();
        delete m_pPlayObjectXfade;
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
        kdDebug() << "[ArtsEngine::loadEffects()] error: !file.open()" << endl;
        return;
    }
     
    QString errorMsg;
    int     errorLine;
    int     errorColumn;       
    if ( !doc.setContent( &file, &errorMsg, &errorLine, &errorColumn ) )
    {
        kdDebug() << "[ArtsEngine::loadEffects()] error: !doc.setContent()" << endl;
        kdDebug() << "[ArtsEngine::loadEffects()] errorMsg   : " << errorMsg    << endl;
        kdDebug() << "[ArtsEngine::loadEffects()] errorLine  : " << errorLine   << endl;
        kdDebug() << "[ArtsEngine::loadEffects()] errorColumn: " << errorColumn << endl;
        file.close();
        return;
    }
            
    QDomElement docElem = doc.documentElement();
    
    for ( QDomNode n = docElem.firstChild(); !n.isNull(); n = n.nextSibling() )
    {
        kdDebug() << "outerloop: " << n.nodeName() << endl;
        kdDebug() << "effectname: " << n.namedItem( "effectname" ).nodeValue() << endl;
        
        for ( QDomNode nAttr = n.firstChild(); !nAttr.isNull(); nAttr = nAttr.nextSibling() )
        {
            if ( nAttr.nodeName() == "attribute" )
            {            
                QString name  = nAttr.namedItem( "name"  ).nodeValue();
                QString type  = nAttr.namedItem( "type"  ).nodeValue();
                QString value = nAttr.namedItem( "value" ).nodeValue();
                
                kdDebug() << "name : " << name  << endl;
                kdDebug() << "type : " << type  << endl;
                kdDebug() << "value: " << value << endl;
                
    /*            createEffect();    
                Arts::DynamicRequest req( *it.data().effect );
                req.method( "_get_" + def.attributes[i].name );
                Arts::Any result;
                result.type = def.attributes[i].type;*/
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
                kdDebug() << "request failed." << endl;
            
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
        kdDebug() << "Arts::Widget gui == NULL! Returning.." << endl;
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
