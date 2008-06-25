/***************************************************************************
* copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
*                                                                         *
*                        Significant parts of this code is inspired       *
*                        and/or copied from KDE Plasma sources, available *
*                        at kdebase/workspace/plasma                      *
*
**************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include "ContextView.h"

#include "Amarok.h"
#include "amarokconfig.h"
#include "Context.h"
#include "ContextScene.h"
#include "DataEngineManager.h"
#include "Debug.h"
#include "Svg.h"
#include "TheInstances.h"
#include "Theme.h"

#include <QFile>
#include <QWheelEvent>

#include <KPluginInfo>
#include <KServiceTypeTrader>
#include <KMenu>

#define DEBUG_PREFIX "ContextView"

namespace Context
{

ContextView* ContextView::s_self = 0;


ContextView::ContextView( Plasma::Containment *cont, QWidget* parent )
    : Plasma::View( cont, parent )
    , EngineObserver( The::engineController() )
    , m_curState( Home )
    , m_appletBrowser( 0 )
    , m_zoomLevel( Plasma::DesktopZoom )
{
    
    s_self = this;

    scene()->setItemIndexMethod( QGraphicsScene::BspTreeIndex );
    //TODO: Figure out a way to use rubberband and ScrollHandDrag
    //setDragMode( QGraphicsView::RubberBandDrag );
    setTransformationAnchor( QGraphicsView::AnchorUnderMouse );
    setCacheMode( QGraphicsView::CacheBackground );
    setInteractive( true );
    setAcceptDrops( true );
    setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    setMouseTracking( true );



    //make background transparent
    QPalette p = palette();
    QColor c = p.color( QPalette::Base );
    c.setAlpha( 0 );
    p.setColor( QPalette::Base, c );
    setPalette( p );

    PERF_LOG( "Accessing Plasma::Theme" );
    // here we initialize all the Plasma paths to Amarok paths
    Plasma::Theme::defaultTheme()->setUseGlobalSettings( false );
    Theme::defaultTheme()->setThemeName( "Amarok-Mockup" );
    PERF_LOG( "Access to Plasma::Theme complete" )
    contextScene()->setAppletMimeType( "text/x-amarokappletservicename" );
    for( int i = 0; i < 3; i++ )
        addContainment();

    setContainment( cont );

    PERF_LOG( "Showing home in contextview" )
    showHome();
    PERF_LOG( "done showing home in contextview" )
}

ContextView::~ContextView()
{
    DEBUG_BLOCK

    // Unload and destroy all Amarok plasma-engines
    const QStringList engines = Plasma::DataEngineManager::self()->listAllEngines();
    foreach( QString engine, engines ) {
        if( engine.startsWith( "amarok-" ) ) {
            debug() << "Unloading plasma engine: " << engine;
            Plasma::DataEngineManager::self()->unloadEngine( engine );
        }
    }
     
    clear( m_curState );
    delete m_appletBrowser;
}

void ContextView::clear( const ContextState& state )
{
    DEBUG_BLOCK
    QString name = "amarok_";

    if( state == Home )
        name += "home";
    else if( state == Current )
        name += "current";
    else
        return; // startup, or some other weird case
    name += "rc";

    // now we save the state, remembering the column info etc
    KConfig appletConfig( name );
    // erase previous config
    foreach( const QString& group, appletConfig.groupList() )
        appletConfig.deleteGroup( group );

    if( contextScene()->containments().size() > 0 )
    {
        DEBUG_LINE_INFO
        Containment* containment = qobject_cast< Containment* >( contextScene()->containments()[0] );
        if( containment )
            containment->saveToConfig( appletConfig );
    }
    contextScene()->clearContainments();
}

//TODO: remove all references to this function
void ContextView::clear()
{
    
}

void ContextView::engineStateChanged( Phonon::State state, Phonon::State oldState )
{
    Q_UNUSED( oldState );
    Q_UNUSED( state );
    
    messageNotify( Current );

   switch( state )
    {
        // Note: This is not as accurate as it should be.  When the user manually changes tracks,
        // the engine sends a StoppedState followed by a PlayingState, causing the view to change.
        // Unfortunately, there is no easy way to fix this.
    case Phonon::PlayingState:
        showCurrentTrack();
        break;

    case Phonon::StoppedState:
        showHome();
        break;

    default:
        ;
    }
}

void ContextView::showHome()
{
//     DEBUG_BLOCK
    if( m_curState == Home)
        return;
    clear( m_curState );
    m_curState = Home;
    loadConfig();
    messageNotify( m_curState );
}

void ContextView::showCurrentTrack()
{
//     DEBUG_BLOCK
    if( m_curState == Current )
        return;
    clear( m_curState );
    m_curState = Current;
    loadConfig();
    messageNotify( Current );
}

// loads applets onto the ContextScene from saved data, using m_curState
void ContextView::loadConfig()
{
//     DEBUG_BLOCK
    QString cur = "amarok_";
    if( m_curState == Home )
        cur += QString( "home" );
    else if( m_curState == Current )
        cur += QString( "current" );
    cur += "rc";

    contextScene()->clearContainments();
    KConfig appletConfig( cur, KConfig::SimpleConfig );
    if( contextScene()->containments().size() > 0 )
    {
        Containment* containment = qobject_cast< Containment* >( contextScene()->containments()[0] );
        if( containment )
            containment->loadConfig( appletConfig );
    }
}

Plasma::Applet* ContextView::addApplet(const QString& name, const QStringList& args)
{
    QVariantList argList;
    QStringListIterator i(args);
    while( i.hasNext() )
        argList << QVariant( i.next() );

    if( containment() )
        return containment()->addApplet( name, argList );
    else
    {
        contextScene()->addContainment( "context" );
        return containment()->addApplet( name, argList );
    }
}

void
ContextView::zoom( Plasma::Containment* containment, Plasma::ZoomDirection direction )
{
    if ( direction == Plasma::ZoomIn )
        zoomIn( containment );
    else if ( direction == Plasma::ZoomOut )
        zoomOut( containment );
}

void
ContextView::zoomIn( Plasma::Containment* toContainment )
{    
    if ( toContainment && containment() != toContainment )
    {
        setContainment( toContainment );
    }
    
    if ( m_zoomLevel == Plasma::GroupZoom )
    {
        setDragMode( NoDrag );
        m_zoomLevel = Plasma::DesktopZoom;
        qreal factor = Plasma::scalingFactor( m_zoomLevel ) / matrix().m11();
        scale( factor, factor );
        if ( containment() )
        {
            //disconnect from other containments
            Plasma::Corona *corona = containment()->corona();
            if ( corona )
            {
                QList<Plasma::Containment*> containments = corona->containments();
                foreach( Plasma::Containment *c, containments )
                {
                    if( c != containment() )
                        disconnectContainment( c );
                }
            }
            setSceneRect( containment()->geometry() );
        }
    }
}

void
ContextView::zoomOut( Plasma::Containment* fromContainment )
{
    if ( m_zoomLevel == Plasma::DesktopZoom )
    {
        m_zoomLevel = Plasma::GroupZoom;
        //connect to other containments
        //FIXME if some other view is zoomed out, a little madness will ensue
        Plasma::Corona *corona = containment()->corona();
        if( corona )
        {
            QList<Plasma::Containment*> containments = corona->containments();
            foreach( Plasma::Containment *c, containments )
            {
                if( c != fromContainment  )
                    connectContainment( c );
            }
        }
        setDragMode( ScrollHandDrag );
        scale( .45, .45 );
        setSceneRect( QRectF( 0, 0, scene()->sceneRect().right(), scene()->sceneRect().bottom() ) );
        ensureVisible( fromContainment->sceneBoundingRect() );
        m_zoomLevel = Plasma::GroupZoom;
    }

}

ContextScene* ContextView::contextScene()
{
    return static_cast<ContextScene*>( scene() );
}


void ContextView::resizeEvent( QResizeEvent* event )
{
    Q_UNUSED( event )
        if ( testAttribute( Qt::WA_PendingResizeEvent ) ) {
            return; // lets not do this more than necessary, shall we?
        }

    scene()->setSceneRect( rect() );

    if( contextScene()->containments().size() > 0 )
    {
        Containment* containment = qobject_cast< Containment* >( contextScene()->containments()[0] );
        if( containment )
            containment->updateSize();
        else
            debug() << "ContextView::resizeEvent NO CONTAINMENT TO UPDATE SIZE! BAD!";
    }

}


void ContextView::wheelEvent( QWheelEvent* event )
{
    if ( event->modifiers() & Qt::ControlModifier )
    {
        if ( event->delta() < 0 )
            zoomOut( containment() );
        else
            zoomIn( containment() );
    }
}


void
ContextView::addContainment()
{
    DEBUG_BLOCK
    Plasma::Corona* corona = containment()->corona();
    if (corona)
    {
        int size = contextScene()->containments().size();
        Plasma::Containment *c = corona->addContainment( "context" );        
        //FIXME: find a better way to resize the containment to a proper size based on the
        //the CV current area size
        int x = 504 * ( size / 2 );
        int y = 604 * ( size % 2 );
        setContainment( c );
        c->resize( 500, 600 );
        c->moveBy( x, y );
        
        debug() << "Containment added at: " << c->geometry();
        debug() << "x,y:" << x << y;
    }

}


void
ContextView::connectContainment( Plasma::Containment* containment )
{
    if( containment )
    {
        connect( containment, SIGNAL( zoomRequested( Plasma::Containment*, Plasma::ZoomDirection ) ),
                this, SLOT( zoom( Plasma::Containment*, Plasma::ZoomDirection ) ) );
        connect( containment, SIGNAL( showAddWidgetsInterface( QPointF ) ),
                this, SLOT( showAppletBrowser() ) );
        connect( containment, SIGNAL( focusRequested( Plasma::Containment* ) ),
                 this, SLOT( setContainment( Plasma::Containment * ) ) );
    }
}

void
ContextView::disconnectContainment( Plasma::Containment* containment )
{
    if( containment )
    {
        disconnect( containment, SIGNAL( zoomRequested( Plasma::Containment*, Plasma::ZoomDirection ) ),
                this, SLOT( zoom( Plasma::Containment*, Plasma::ZoomDirection ) ) );
        disconnect( containment, SIGNAL( showAddWidgetsInterface( QPointF ) ),
                this, SLOT( showAppletBrowser() ) );
        disconnect( containment, SIGNAL( focusRequested( Plasma::Containment* ) ),
                 this, SLOT( setContainment( Plasma::Containment * ) ) );
    }
}

void
ContextView::setContainment( Plasma::Containment* containment )
{
    if( containment != this->containment() )
    {
        disconnectContainment( this->containment() );
        connectContainment( containment );
        Plasma::View::setContainment( containment );
    }    
}

void
ContextView::nextContainment()
{
    QList<Plasma::Containment*> containments = contextScene()->containments();
    int index = containments.indexOf( containment() );
    index = ( index + 1 ) % containments.size();
    setContainment( containments.at( index ) );
}

void
ContextView::previousContainment()
{
    QList<Plasma::Containment*> containments = contextScene()->containments();
    int index = containments.indexOf( containment() );
    index = ( index - 1 ) % containments.size();
    setContainment( containments.at( index ) );
}

void
ContextView::showAppletBrowser()
{
    DEBUG_BLOCK
    if( !containment() )
        return;

    if( !m_appletBrowser )
    {
        m_appletBrowser = new Plasma::AppletBrowser();
        m_appletBrowser->setContainment( containment() );
        m_appletBrowser->setApplication( "amarok" );
        m_appletBrowser->setAttribute( Qt::WA_DeleteOnClose );
        m_appletBrowser->setWindowTitle( i18n( "Add Applets" ) );
        connect( m_appletBrowser, SIGNAL( destroyed() ), this, SLOT( appletBrowserDestroyed() ) );
        m_appletBrowser->show();
    }
    else
    {
        m_appletBrowser->setContainment( containment() );
        m_appletBrowser->activateWindow();
        m_appletBrowser->raise();
    }    
}

void
ContextView::appletBrowserDestroyed()
{
    m_appletBrowser = 0;
}

} // Context namespace

#include "ContextView.moc"
