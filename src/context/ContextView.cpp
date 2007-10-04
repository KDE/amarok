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

#include "amarok.h"
#include "amarokconfig.h"
#include "Context.h"
#include "ContextScene.h"
#include "DataEngineManager.h"
#include "debug.h"
#include "enginecontroller.h"
#include "Svg.h"
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
    , EngineObserver( EngineController::instance() )
    , m_columns( 0 )
{
    DEBUG_BLOCK

    s_self = this;

//     setFrameShape( QFrame::NoFrame );
    setAutoFillBackground( true );

    setScene( new ContextScene( rect(), this ) );
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

    // here we initialize all the Plasma paths to Amarok paths
    Theme::self()->setApplication( "amarok" );
    contextScene()->setAppletMimeType( "text/x-amarokappletservicename" );

    contextScene()->loadDefaultSetup();
    
    createContainment();

    connect(scene(), SIGNAL( appletRemoved( QObject * ) ), m_columns, SLOT( appletRemoved( QObject* ) ) );


    showHome();
}

ContextView::~ContextView()
{
    DEBUG_BLOCK
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
        return; // startup, or some other wierd case
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
    contextScene()->clearApplets();
}


void ContextView::engineStateChanged( Engine::State state, Engine::State oldState )
{
    DEBUG_BLOCK
    Q_UNUSED( oldState ); Q_UNUSED( state );

    switch( state )
    {
    case Engine::Playing:
        showCurrentTrack();
        break;

    case Engine::Empty:
        showHome();
        break;

    default:
        ;
    }
}

void ContextView::showHome()
{
    DEBUG_BLOCK
    clear( m_curState );
    m_curState = Home;
    loadConfig();
    messageNotify( m_curState );
}

void ContextView::showCurrentTrack()
{
    DEBUG_BLOCK
    clear( m_curState );
    m_curState = Current;
    loadConfig();
    messageNotify( Current );
}

// loads applets onto the ContextScene from saved data, using m_curState
void ContextView::loadConfig()
{
    DEBUG_BLOCK
    QString cur = "amarok_";
    if( m_curState == Home )
        cur += QString( "home" );
    else if( m_curState == Current )
        cur += QString( "current" );
    cur += "rc";

    contextScene()->clearApplets();
    KConfig appletConfig( cur, KConfig::OnlyLocal );
    if( contextScene()->containments().size() > 0 ) 
    {
        Containment* containment = qobject_cast< Containment* >( contextScene()->containments()[0] );
        if( containment )
            containment->loadConfig( appletConfig );
    }
}

void ContextView::engineNewMetaData( const MetaBundle&, bool )
{
    ; //clear();
}


Applet* ContextView::addApplet(const QString& name, const QStringList& args)
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
    DEBUG_BLOCK
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
