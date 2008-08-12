/*****************************************************************************
* copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>           *
*                      : (C) 2008 William Viana Soares <vianasw@gmail.com>   *
*                        Significant parts of this code is inspired          *
*                        and/or copied from KDE Plasma sources, available    *
*                        at kdebase/workspace/plasma                         *
*
******************************************************************************/

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


ContextView::ContextView( Plasma::Containment *cont, Plasma::Corona *corona, QWidget* parent )
//     : Plasma::View( cont, parent )
    : QGraphicsView( corona, parent )
    , EngineObserver( The::engineController() )
    , m_curState( Home )
    , m_appletBrowser( 0 )
    , m_zoomLevel( Plasma::DesktopZoom )
    , m_startupFinished( false )
    , m_toolBoxAdded( false )
{
    s_self = this;

//     scene()->setItemIndexMethod( QGraphicsScene::BspTreeIndex );
    //TODO: Figure out a way to use rubberband and ScrollHandDrag
    //setDragMode( QGraphicsView::RubberBandDrag );
    setTransformationAnchor( QGraphicsView::NoAnchor );
    setCacheMode( QGraphicsView::CacheBackground );
    setInteractive( true );
    setAcceptDrops( true );
    setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    setMouseTracking( true );

    scene()->setItemIndexMethod( QGraphicsScene::NoIndex );

    connectContainment( cont );
    setContainment( cont );
    
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
    Containment* amarokContainment = qobject_cast<Containment* >( cont );    
    if( amarokContainment )
    {
        amarokContainment->setView( this );
        amarokContainment->setTitle( "Context #0" );    
        amarokContainment->addCurrentTrack();
    }

    PERF_LOG( "Showing home in contextview" )
    showHome();
    PERF_LOG( "done showing home in contextview" )
    
    m_startupFinished = true;
    
}

ContextView::~ContextView()
{
    DEBUG_BLOCK

    // Unload and destroy all Amarok plasma-engines
    const QStringList engines = Plasma::DataEngineManager::self()->listAllEngines();
    foreach( QString engine, engines ) {
        if( engine.startsWith( "amarok-" ) ) {
            debug() << "Unloading plasma engine: " << engine;

            // PlasmaDataEngineManager uses refcounting for the engines, so we need to unload until the refcount reaches 0
            while( Plasma::DataEngineManager::self()->engine( engine )->isValid() )
                Plasma::DataEngineManager::self()->unloadEngine( engine );
        }
    }
     
    clear( m_curState );
    delete m_appletBrowser;
}


void ContextView::clear( const ContextState& state )
{
    Q_UNUSED( state )
    DEBUG_BLOCK

    QString name = "amarok_homerc";
    // now we save the state, remembering the column info etc
    KConfig appletConfig( name );
    // erase previous config
    foreach( const QString& group, appletConfig.groupList() )
        appletConfig.deleteGroup( group );
    int numContainments = contextScene()->containments().size();
    for(int i = 0; i < numContainments; i++ )
    {
        DEBUG_LINE_INFO
        Containment* containment = qobject_cast< Containment* >( contextScene()->containments()[i] );
        KConfigGroup cg( &appletConfig, QString( "Containment %1" ).arg( i ) );
       if( containment )
           containment->saveToConfig( cg );
    }
    contextScene()->clearContainments();
}

//TODO: remove all references to this function
void ContextView::clear()
{
    
}

// void
// ContextView::mousePressEvent( QMouseEvent* event )
// {
//     DEBUG_BLOCK
//     // event->accept();
//     QPoint pos = mapToScene( event->pos() ).toPoint();
//     debug() << "Event pos: " << event->pos();
//     debug() << "mapFromScene pos: " << mapFromScene( event->pos() );
//     debug() << "mapToScene pos: " << mapToScene( event->pos() );
//     debug() << "sceneRect: " << sceneRect();
//     debug() << "view items at pos: " << items( event->pos() ).count();
//     debug() << "scene items: " << scene()->items( mapToScene( event->pos() ) ).count();
// //     if( itemAt( pos ) )    
//     foreach( QGraphicsItem* item, items( event->pos() ) )
//     {
//         debug() << "got an item under click, seeing if it is an applet, and its sceneBoundingRect: " << itemAt( event->pos() )->sceneBoundingRect();
//         Plasma::Applet* a = dynamic_cast<Plasma::Applet* >( item );
//         if( a )
//         {
//             debug() << "cast successful";
// 
// //             if( a->isContainment() )
// //             {
// //                 Plasma::Containment* c = dynamic_cast<Plasma::Containment* >( a );
// //                 setContainment( c );
// //             }
// //             else if( a->containment() )
// //                 setContainment( a->containment() );
//         }
//     }
//     debug() << "scene rect:" << scene()->sceneRect();
//     QGraphicsView::mousePressEvent( event );
// }

void ContextView::engineStateChanged( Phonon::State state, Phonon::State oldState )
{
    Q_UNUSED( oldState );
    Q_UNUSED( state );

    if( state == Phonon::PlayingState )
        messageNotify( Current );
    else if( state == Phonon::StoppedState )
        messageNotify( Home );
}

void ContextView::showHome()
{
    DEBUG_BLOCK

    m_curState = Home;
    loadConfig();
    messageNotify( m_curState );
}


// loads applets onto the ContextScene from saved data, using m_curState
void ContextView::loadConfig()
{
    contextScene()->clearContainments();    

    int numContainments = contextScene()->containments().size();
    KConfig conf( "amarok_homerc", KConfig::SimpleConfig );
    for( int i = 0; i < numContainments; i++ )
    {
        Containment* containment = qobject_cast< Containment* >( contextScene()->containments()[i] );
        if( containment )
        {
            KConfigGroup cg( &conf, QString( "Containment %1" ).arg( i ) );
            containment->loadConfig( cg );
        }
    }
}

Plasma::Applet* ContextView::addApplet( const QString& name, const QStringList& args )
{
    QVariantList argList;
    QStringListIterator i(args);
    while( i.hasNext() )
        argList << QVariant( i.next() );

    if( !containment() )
        contextScene()->addContainment( "context" );

    return containment()->addApplet( name, argList );
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
    DEBUG_BLOCK
    if ( toContainment && containment() != toContainment )
    {
        setContainment( toContainment );
    }
    
    if ( m_zoomLevel == Plasma::GroupZoom )
    {
        connect( Plasma::Animator::self(), SIGNAL( customAnimationFinished( int ) ),
                 this, SLOT( zoomInFinished( int ) ) );
        Plasma::Animator::self()->customAnimation( 120, 800, Plasma::Animator::EaseInOutCurve,
                                                    this, "animateZoomIn" );
    }

}

void
ContextView::zoomOut( Plasma::Containment* fromContainment )
{
    Q_UNUSED( fromContainment )
    if ( m_zoomLevel == Plasma::DesktopZoom )
    {        
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
        Plasma::Animator::self()->customAnimation( 120, 800, Plasma::Animator::EaseInOutCurve,
                                                            this, "animateZoomOut" );                                                                    
    }

}


void
ContextView::animateZoomIn( qreal progress, int id )
{
    Q_UNUSED( id )

    if( progress > 0 )
    {
        qreal s = ( progress / 2.0 + 0.5 ) / matrix().m11();
        centerOnZoom( s, Plasma::ZoomIn );        
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
    
    int numContainments = contextScene()->containments().size();
    for( int i = 0; i < numContainments; i++ )
    {
        Containment* containment = qobject_cast< Containment* >( contextScene()->containments()[i] );
        if( containment )
            containment->correctToolBoxPos();
    }
}

void
ContextView::animateZoomOut( qreal progress, int id )
{
    Q_UNUSED( id )

    qreal s =  ( 1.0 - progress / 1.8 ) / matrix().m11();
    centerOnZoom( s, Plasma::ZoomOut );
}

void
ContextView::zoomOutFinished( int id )
{
    Q_UNUSED( id )
    DEBUG_BLOCK

    m_zoomLevel = Plasma::GroupZoom;
//     setDragMode( ScrollHandDrag );
    
//     setSceneRect( mapToScene( rect() ).boundingRect() );
    setSceneRect( QRectF() );
    debug() << "sceneRect: " << sceneRect();
    ensureVisible( rect(), 0, 0 );
    
    disconnect( Plasma::Animator::self(), SIGNAL( customAnimationFinished( int ) ), this, SLOT( zoomOutFinished( int ) ) );
    int numContainments = contextScene()->containments().size();
    for( int i = 0; i < numContainments; i++ )
    {
        Containment* containment = qobject_cast< Containment* >( contextScene()->containments()[i] );
        if( containment )
            containment->correctToolBoxPos();
    }
}

Plasma::ZoomLevel
ContextView::zoomLevel() const
{
    return m_zoomLevel;
}

void
ContextView::centerOnZoom( qreal sFactor, Plasma::ZoomDirection direction )
{ 
    qreal left, top, right, bottom;
    
    qreal width = sceneRect().width();
    qreal height = sceneRect().height();
    
    const QPointF topLeft = containment()->geometry().topLeft();
    const QPointF topRight = containment()->geometry().topRight();
    const QPointF bottomLeft = containment()->geometry().bottomLeft();

    const qreal x = qMax( qreal(0.0), sceneRect().topRight().x() - ( width * 1/sFactor ) );
    const qreal y = qMax( qreal(0.0), sceneRect().bottomLeft().y() - ( height * 1/sFactor ) );

    left = qMin(  topLeft.x(), x ) ;
    top = qMin( topLeft.y() , y  );
    right = qMax( topRight.x(), width * 1/sFactor );
    bottom = qMax( bottomLeft.y(), height * 1/sFactor );

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
    if( !m_toolBoxAdded )
    {
        m_toolBoxAdded = true;
        int numContainments = contextScene()->containments().size();
        for( int i = 0; i < numContainments; i++ )
        {
            Containment* containment = qobject_cast< Containment* >( contextScene()->containments()[i] );
            if( containment )
                containment->addToolBox();
        }
    }
}


void
ContextView::updateContainmentsGeometry()
{
    DEBUG_BLOCK

    debug() << "cv rect: " << rect();
    int x,y;
    const int last = contextScene()->containments().size() - 1;
    const int width = rect().width();
    const int height = rect().height();

    if( m_zoomLevel == Plasma::DesktopZoom )
    {
        for( int i = last; i >= 0; i-- )
        {
            Containment* containment = qobject_cast< Containment* >( contextScene()->containments()[i] );
            
            x = ( width + 25 ) * ( i % 2 );
            y = ( height + 65 )* ( i / 2 );
            QRectF newGeom( rect().topLeft().x() + x,
                                    rect().topLeft().y() + y,
                                    width + 20 ,
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
    QGraphicsView::wheelEvent( event );
}


void
ContextView::addContainment()
{
    DEBUG_BLOCK

    Plasma::Corona* corona = containment()->corona();
    if (corona)
    {
        const int size = contextScene()->containments().size();
        Plasma::Containment *c = corona->addContainment( "context" );
        c->setScreen( 0 );
        c->setFormFactor( Plasma::Planar );
        
        const int x = ( rect().width() + 25 ) * ( size % 2 );
        const int y = ( rect().height() + 65 ) * ( size / 2 );

        Containment* containment = qobject_cast< Containment* >( c );

        QRectF newGeom( rect().topLeft().x() + x,
                                rect().topLeft().y() + y,
                                rect().width() + 20,
                                rect().height() + 60 );
        if( containment )
        {
            containment->setView( this );
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
        DEBUG_LINE_INFO
//         disconnectContainment( this->containment() );
        if( containment->isContainment() )
        {
            if( m_startupFinished )
                m_startPos = this->containment()->geometry();

                                                              
//             //This call will mess with the current scene geometry
//             setContainment( containment );
            m_containment = containment;

            //this disconnect prevents from undesired containment's geometry change
            disconnect( containment, SIGNAL( geometryChanged() ), 0, 0 );
            
            //resize the containment and the scene to an appropriate size
            qreal left, top, right, bottom;
            containment->getContentsMargins( &left, &top, &right, &bottom );

            QSizeF correctSize( rect().size().width() + left - right , rect().size().height() + top - bottom );
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
            
            if( m_startupFinished && m_zoomLevel == Plasma::DesktopZoom )
            {                
                m_destinationPos = containment->geometry();                               

                Plasma::Animator::self()->customAnimation( m_startPos.width() / 8, 800,
                                                            Plasma::Animator::EaseInOutCurve,                                                           
                                                            this, "animateContainmentChange" );
                debug() << "startPos: " << m_startPos;
                debug() << "destinationPos: " << m_destinationPos;
            }
        }
    }
    
}

void
ContextView::nextContainment()
{
    const QList<Plasma::Containment*> containments = contextScene()->containments();
    int index = containments.indexOf( containment() );
    index = ( index + 1 ) % containments.size();

    setContainment( containments.at( index ) );
}

void
ContextView::previousContainment()
{
    const QList<Plasma::Containment*> containments = contextScene()->containments();
    int index = containments.indexOf( containment() );
    index = ( index - 1 ) % containments.size();

    setContainment( containments.at( index ) );
}

void
ContextView::animateContainmentChange( qreal progress, int id )
{
    DEBUG_BLOCK
    Q_UNUSED( id )

    qreal incrementX;
    qreal incrementY;
    
    qreal left, top, right, bottom;
    
    containment()->getContentsMargins( &left, &top, &right, &bottom );
    
    qreal x = m_destinationPos.left() + left;    
    qreal y = m_destinationPos.top() + top;
        
    if( m_startPos.left() < m_destinationPos.left() )
    {
        incrementX = progress * ( m_destinationPos.width() + left + 5 );
        x = m_startPos.left() + incrementX;
    }
    else if ( m_startPos.left() > m_destinationPos.left() )
    {
        incrementX = progress * ( m_destinationPos.width() + 5 );
        x = m_startPos.left() - incrementX;
    }
    if( m_startPos.top() < m_destinationPos.top() )
    {
        incrementY = progress * ( m_destinationPos.height() + top + 5 );
        y = m_startPos.top() + incrementY;
    }
    else if( m_startPos.top() > m_destinationPos.top() )
    {
        incrementY = progress * ( m_destinationPos.height() + 5 );
        y = m_startPos.top() - incrementY;
    }
    
    QRectF visibleRect( QPointF( x, y ), m_destinationPos.size() );
    setSceneRect( visibleRect );
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

Plasma::Containment *
ContextView::containment()
{
    return m_containment;
}

} // Context namespace

#include "ContextView.moc"
