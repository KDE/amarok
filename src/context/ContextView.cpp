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
#include "debug.h"
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


ContextView::ContextView( QWidget* parent )
    : QGraphicsView( parent )
    , EngineObserver( The::engineController() )
    , m_columns( 0 )
    , m_curState( Home )
{

    s_self = this;

    PERF_LOG( "Creating contextScene" )
    setScene( new ContextScene( this ) );
    PERF_LOG( "Created ContextScene" )
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

    PERF_LOG( "Accessing Plasma::Theme" );
    // here we initialize all the Plasma paths to Amarok paths
    Theme::defaultTheme()->setThemeName( "Amarok-Mockup" );
    PERF_LOG( "Access to Plasma::Theme complete" )
    contextScene()->setAppletMimeType( "text/x-amarokappletservicename" );

    PERF_LOG( "Loading default contextScene" )
    contextScene()->loadDefaultSetup();
    PERF_LOG( "Loaded default contextScene" )

    PERF_LOG( "Creating containment" )
    createContainment();
    PERF_LOG( "Created containment" )

    connect(scene(), SIGNAL( appletRemoved( QObject * ) ), m_columns, SLOT( appletRemoved( QObject* ) ) );

    PERF_LOG( "Showing home in contextview" )
    showHome();
    PERF_LOG( "done showing home in contextview" )
}

ContextView::~ContextView()
{
    clear( m_curState );
}

void ContextView::clear()
{
    delete m_columns;
}

void ContextView::clear( const ContextState& state )
{
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
        Containment* containment = qobject_cast< Containment* >( contextScene()->containments()[0] );
        if( containment )
            containment->saveToConfig( appletConfig );
    }
    contextScene()->clearContainments();
}


void ContextView::engineStateChanged( Phonon::State state, Phonon::State oldState )
{
    Q_UNUSED( oldState );

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

    if( m_columns )
        return m_columns->addApplet( name, argList );
    else
    {
        createContainment();
        return m_columns->addApplet( name, argList );
    }
}

void ContextView::zoomIn()
{
    //TODO: Change level of detail when zooming
    // 10/8 == 1.25
    scale( 1.25, 1.25 );
}

void ContextView::zoomOut()
{
    // 8/10 == .8
    scale( .8, .8 );
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
    }
}

void ContextView::wheelEvent( QWheelEvent* event )
{
    if ( scene() && scene()->itemAt( event->pos() ) ) {
        QGraphicsView::wheelEvent( event );
        return;
    }

    if ( event->modifiers() & Qt::ControlModifier ) {
        if ( event->delta() < 0 ) {
            zoomOut();
        } else {
            zoomIn();
        }
    }
}

void ContextView::createContainment()
{
    if( contextScene()->containments().size() == 0 ) // haven't created it yet
    {
        // yes, this is ugly :(
        m_columns = dynamic_cast< Containment* >
            (  contextScene()->addContainment( "context" ) );
    }
}

} // Context namespace

#include "ContextView.moc"
