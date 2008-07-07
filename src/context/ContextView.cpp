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
    , m_factor( 1 )
{
    
    s_self = this;

    scene()->setItemIndexMethod( QGraphicsScene::BspTreeIndex );
    //TODO: Figure out a way to use rubberband and ScrollHandDrag
    //setDragMode( QGraphicsView::RubberBandDrag );
    setTransformationAnchor( QGraphicsView::NoAnchor );
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
    
    connectContainment( cont );
    setContainment( cont );
    Containment* amarokContainment = qobject_cast<Containment* >( cont );    
    if( amarokContainment )
        amarokContainment->setTitle( "Context #0" );
    
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

void
ContextView::mousePressEvent( QMouseEvent* event )
{
    DEBUG_BLOCK
    QPointF pos = mapToScene( event->pos() );
    debug() << "Event pos: " << event->pos();
    debug() << "mapFromScene pos: " << mapFromScene( event->pos() );
    debug() << "mapToScene pos: " << mapToScene( event->pos() );
    if( scene()->itemAt( pos ) )
    {
        Plasma::Applet* a = dynamic_cast<Plasma::Applet* >( scene()->itemAt( event->pos() ) );
        if( a )
        {
            debug() << "cast successful";

            if( a->isContainment() )
            {
                Plasma::Containment* c = dynamic_cast<Plasma::Containment* >( a );
                setContainment( c );
            }
            else if( a->containment() )
                setContainment( a->containment() );
        }
    }
    else
    {
        debug() << "OUTside item";
    }
    debug() << "scene rect:" << scene()->sceneRect();
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
        connect( Plasma::Animator::self(), SIGNAL( customAnimationFinished( int ) ),
                 this, SLOT( zoomInFinished( int ) ) );
        Plasma::Animator::self()->customAnimation( 35, 1000, Plasma::Animator::LinearCurve, this, "animateZoomIn" );
    }
    //HACK: this is the only way that the containments resize correctly after
    //the CV widget has ben resized while in zoom level Plasma::GroupZoom
    resize( size().width()+1, size().height() );
    resize( size().width()-1, size().height() );
}

void
ContextView::zoomOut( Plasma::Containment* fromContainment )
{
    Q_UNUSED( fromContainment )
    if ( m_zoomLevel == Plasma::DesktopZoom )
    {

        m_factor = 1;
        int count = contextScene()->containments().size();
        for( int i = 0; i < count; i++ )
        {
            Containment* containment = qobject_cast< Containment* >( contextScene()->containments()[i] );
            if( containment )
            {
                containment->showTitle();
            }
        }
        
        connect( Plasma::Animator::self(), SIGNAL( customAnimationFinished( int ) ),
                 this, SLOT( zoomOutFinished( int ) ) );
        Plasma::Animator::self()->customAnimation( 35, 1000, Plasma::Animator::EaseInCurve, this, "animateZoomOut" );

    }

}

void
ContextView::animateZoomIn( qreal progress, int id )
{
    Q_UNUSED( progress )
    if( m_factor < 1 )
    {
        m_factor += 0.08;
        qreal s = m_factor / matrix().m11();
        centerOnZoom( s, Plasma::ZoomIn );
    }
    else
    {
        Plasma::Animator::self()->stopCustomAnimation( id );
        zoomInFinished( id );
    }
}

void
ContextView::zoomInFinished( int id )
{
    Q_UNUSED( id )
    int count = contextScene()->containments().size();
    for( int i = 0; i < count; i++ )
    {
        Containment* containment = qobject_cast< Containment* >( contextScene()->containments()[i] );
        if( containment )
        {
            containment->hideTitle();
        }
    }
    m_zoomLevel = Plasma::DesktopZoom;
    setDragMode( NoDrag );
    disconnect( Plasma::Animator::self(), SIGNAL( customAnimationFinished( int ) ),
                 this, SLOT( zoomInFinished( int ) ) );
    resize( size().width()+1, size().height() );
    resize( size().width()-1, size().height() );
    m_factor = 1;
}

void
ContextView::animateZoomOut( qreal progress, int id )
{
    Q_UNUSED( progress )
  
    if( m_factor > 0.45 )
    {
        m_factor -= 0.08;
        qreal s = m_factor / matrix().m11();
        centerOnZoom( s, Plasma::ZoomOut );

    }
    else
    {
        Plasma::Animator::self()->stopCustomAnimation( id );
        zoomOutFinished( id );
    }
}

void
ContextView::zoomOutFinished( int id )
{
    DEBUG_BLOCK
    Q_UNUSED( id )
    m_zoomLevel = Plasma::GroupZoom;
    setDragMode( ScrollHandDrag );
    
    setSceneRect( mapToScene( rect() ).boundingRect() );
    ensureVisible( rect(), 0, 0 );
    
    disconnect( Plasma::Animator::self(), SIGNAL( customAnimationFinished( int ) ),
                 this, SLOT( zoomOutFinished( int ) ) );
    m_factor = 0.45;
}


void
ContextView::centerOnZoom( qreal sFactor, Plasma::ZoomDirection direction )
{

    qreal left, top, right, bottom;
    
    qreal width = sceneRect().width();
    qreal height = sceneRect().height();
    
    QPointF topLeft = containment()->geometry().topLeft();
    QPointF topRight = containment()->geometry().topRight();
    QPointF bottomLeft = containment()->geometry().bottomLeft();

    qreal x = qMax( 0.0, sceneRect().topRight().x() - ( width * 1/sFactor ) );
    qreal y = qMax( 0.0, sceneRect().bottomLeft().y() - ( height * 1/sFactor ) );

    left = qMin(  topLeft.x(), x ) ;
    top = qMin( topLeft.y() , y  );
    right = qMax( topRight.x(), 25 + width * 1/sFactor );
    bottom = qMax( bottomLeft.y(), 65 + height * 1/sFactor );

    QRectF visibleRect( QPoint( left, top ), QPoint( right, bottom ) );
    scale( sFactor, sFactor );
    if( direction == Plasma::ZoomIn )
    {
        containment()->getContentsMargins( &left, &top, &right, &bottom );
        setSceneRect( visibleRect.adjusted( left, top, -right, -bottom )  );
    }
    else
    {
        setSceneRect( visibleRect );
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
    updateContainmentsGeometry();
}


void
ContextView::updateContainmentsGeometry()
{
    
    int last = contextScene()->containments().size() - 1;
    int x,y;
    int width = rect().width();
    int height = rect().height();

    if( m_zoomLevel == Plasma::DesktopZoom )
    {
        
        for( int i = last; i >= 0; i-- )
        {
            Containment* containment = qobject_cast< Containment* >( contextScene()->containments()[i] );
            
            x = ( width + 25 ) * ( i % 2 );
            y = ( height + 65 )* ( i / 2 );
            QRectF newGeom( rect().topLeft().x() + x,
                                    rect().topLeft().y() + y,
                                    width + 20,
                                    height + 60 );
            if( containment )
            {
                containment->updateSize( newGeom );

            }
            else
                debug() << "ContextView::resizeEvent NO CONTAINMENT TO UPDATE SIZE! BAD!";
        }
        qreal left, top, right, bottom;
        containment()->getContentsMargins( &left, &top, &right, &bottom );
        QRectF contRect( containment()->geometry() );
        setSceneRect( contRect.adjusted( left, top, -right, -bottom ) );
    }
}

void ContextView::wheelEvent( QWheelEvent* event )
{
    if ( event->modifiers() & Qt::ControlModifier && !Plasma::Animator::self()->isAnimating() )
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
        c->setScreen( 0 );
        c->setFormFactor( Plasma::Planar );
        
        int x = ( rect().width() + 30 ) * ( size % 2 );
        int y = ( rect().height() + 70 ) * ( size / 2 );

        Containment* containment = qobject_cast< Containment* >( c );

        QRectF newGeom( rect().topLeft().x() + x,
                                rect().topLeft().y() + y,
                                rect().width() + 20,
                                rect().height() + 60 );
        if( containment )
        {
            containment->updateSize( newGeom );
            containment->setTitle( QString( "Context #%1" ).arg( size ) );
        }
        else
            debug() << "ContextView::resizeEvent NO CONTAINMENT TO UPDATE SIZE! BAD!";
        
        connectContainment( c );
        setContainment( c );
        
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
        Containment* amarokContainment = qobject_cast<Containment*>( containment );
        if( amarokContainment )
        {
            connect( amarokContainment, SIGNAL( appletRejected( QString, int ) ),
                     this, SLOT( findContainmentForApplet( QString, int ) ) );
        }
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
        Containment* amarokContainment = qobject_cast<Containment*>( containment );
        if( amarokContainment )
        {
            disconnect( amarokContainment, SIGNAL( appletRejected( QString, int ) ),
                     this, SLOT( findContainmentForApplet( QString, int ) ) );
        }         
    }
}

void
ContextView::setContainment( Plasma::Containment* containment )
{
    DEBUG_BLOCK
    if( containment != this->containment() )
    {
//         disconnectContainment( this->containment() );
        if( containment->isContainment() )
        {

            //This call will mess with the current scene geometry
            Plasma::View::setContainment( containment );            

            //this disconnect prevents from undesired containment's geometry change
            disconnect( containment, SIGNAL( geometryChanged() ), 0, 0 );
            
            //resize the containment and the scene to an appropriate size
            
            qreal left, top, right, bottom;
            containment->getContentsMargins( &left, &top, &right, &bottom );
            QSizeF correctSize( rect().size().width() + left - right , rect().size().height() + top - bottom);
            
            if( m_zoomLevel == Plasma::DesktopZoom )
            {                
                containment->resize( correctSize );
                setSceneRect( containment->contentsRect() );                
            }
            else
            {
                containment->resize( correctSize );
                debug() << "correct size: " << correctSize;
                QRectF correctRect( 0, 0,
                                    mapToScene( rect() ).boundingRect().width(),
                                    mapToScene( rect() ).boundingRect().height() );
                setSceneRect( correctRect );
                debug() << "setSceneRect: " <<  mapToScene( rect() ).boundingRect() ;
            }

            if( m_appletBrowser )
                m_appletBrowser->setContainment( containment );

        }
        
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
ContextView::findContainmentForApplet( QString pluginName, int rowSpan )
{
    DEBUG_BLOCK
    Plasma::Corona *corona = containment()->corona();
    if ( corona )
    {
        QList<Plasma::Containment*> containments = corona->containments();
        bool placeFound = false;
        int count = containments.count();
        int i = 0;
        while( !placeFound && i < count )
        {
            Containment* amarokContainment = qobject_cast<Containment*>( containments[i] );
            if( amarokContainment )
            {
                if( amarokContainment->hasPlaceForApplet( rowSpan ) )
                {
                    
                    amarokContainment->addApplet( pluginName );
                    
                    setContainment( amarokContainment );
                    if( m_zoomLevel == Plasma::DesktopZoom )
                    {
                        //HACK alert!
                        resize( size().width()+1, size().height() );
                        resize( size().width()-1, size().height() );                        
                    }
                    
                    placeFound = true;
                }
            }
            i++;
        }
        
        if( !placeFound )
        {
            debug() << "No availiable place to add " << pluginName << " applet";
            debug() << "Create new containment and add it there";
        }

    }
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
