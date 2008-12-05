/*****************************************************************************
* copyright            : (C) 2007-2008 Leo Franchi <lfranchi@gmail.com>      *
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
#include "Context.h"
#include "ContextScene.h"
#include "DataEngineManager.h"
#include "Debug.h"
#include "Svg.h"
#include "Theme.h"
#include "amarokconfig.h"

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
    , m_zoomLevel( Plasma::DesktopZoom )
    , m_startupFinished( false )
    , m_containment( 0 )
    , m_numContainments( 4 )
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
        // now add the appropriate arrows
        // HACK assuming 4 containments in grid layout---but since everywhere else in this code
        // this is assumed, we have bigger problems if we want to change that.
        
    for( int i = 0; i < m_numContainments - 1; i++ )
    {
        QVariantList args;
        if( i == 0 )
        {
            args.append( RIGHT );
            args.append( DOWN );
        } else if( i == 1 )
        {
            args.append( LEFT );
            args.append( DOWN );
        } else if( i == 2 )
        {
            args.append( RIGHT );
            args.append( UP );
        } else if( i == 3 )
        {
            args.append( LEFT );
            args.append( UP );
        }
        addContainment( args );    
    }
    
    setContainment( cont );
    cont->setPos( 0, 0 );
    cont->updateConstraints();
    Containment* amarokContainment = qobject_cast<Containment* >( cont );    
    if( amarokContainment )
    {
        amarokContainment->setView( this );
        amarokContainment->setTitle( i18n( "Page #1" ) );
        amarokContainment->setFooter( "1" );
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
    foreach( const QString &engine, engines ) {
        if( engine.startsWith( "amarok-" ) ) {
            debug() << "Unloading plasma engine: " << engine;

            // PlasmaDataEngineManager uses refcounting for the engines, so we need to unload until the refcount reaches 0
            while( Plasma::DataEngineManager::self()->engine( engine )->isValid() )
                Plasma::DataEngineManager::self()->unloadEngine( engine );
        }
    }
     
    clear( m_curState );
    //this should be done to prevent a crash on exit
    clearFocus();
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
    DEBUG_BLOCK
    /*
    if( state == Phonon::PlayingState )
        debug() << "got state change to playing: state";
    else if( state == Phonon::StoppedState )
        debug() << "got state change to stopped"; 
    */    
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
    // tell the containments about its zoom status
    Containment* containment = qobject_cast< Containment* >( toContainment );
    if( containment )
        containment->setZoomLevel( Plasma::DesktopZoom );
        
    if ( m_zoomLevel == Plasma::GroupZoom )
    {        
        m_zoomLevel = Plasma::DesktopZoom;
        qreal factor = Plasma::scalingFactor( m_zoomLevel ) / matrix().m11();
        scale( factor, factor );
        updateContainmentsGeometry();
        int count = contextScene()->containments().size();
        for( int i = 0; i < count; i++ )
        {
            Containment* containment = qobject_cast< Containment* >( contextScene()->containments()[i] );
            if( containment )
            {
                containment->hideTitle();
            }
        }            
    }

}

void
ContextView::zoomOut( Plasma::Containment* fromContainment )
{
    DEBUG_BLOCK
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
                containment->setZoomLevel( Plasma::GroupZoom );
            }
        }
        m_zoomLevel = Plasma::GroupZoom;
        updateContainmentsGeometry( true );
        debug() << "Scaling factor: " << Plasma::scalingFactor( m_zoomLevel );
        qreal factor = Plasma::scalingFactor( m_zoomLevel ) - 0.05;
        qreal s = factor / matrix().m11();
        scale( s, s );

        setSceneRect( QRectF() );
        ensureVisible( rect(), 0, 0 );
                                                           
    }

}


void
ContextView::animateZoomIn( qreal progress, int id )
{
    Q_UNUSED( id )

    if( progress > 0 )
    {
        qreal s = ( progress / 2.0 + 0.5 ) / matrix().m11();
 //       debug() << "matrix().m11():" << matrix().m11() <<  "progress:" << progress << "s: " << s;
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
    disconnect( Plasma::Animator::self(), SIGNAL( customAnimationFinished( int ) ), this, SLOT( zoomInFinished( int ) ) );
}

void
ContextView::animateZoomOut( qreal progress, int id )
{
    Q_UNUSED( id )

    qreal s =  ( 1.0 - progress / 1.8 ) / matrix().m11();
//    debug() << "matrix().m11():" << matrix().m11() <<  "progress:" << progress << "s: " << s;
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
//    debug() << "sceneRect: " << sceneRect();
    ensureVisible( rect(), 0, 0 );
    
    disconnect( Plasma::Animator::self(), SIGNAL( customAnimationFinished( int ) ), this, SLOT( zoomOutFinished( int ) ) );
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
 //   debug() << "setting sceneRect to:" << visibleRect;
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
    DEBUG_BLOCK
    Q_UNUSED( event )
       
    if ( testAttribute( Qt::WA_PendingResizeEvent ) ) {
        return; // lets not do this more than necessary, shall we?
    }

    updateContainmentsGeometry();

}


void
ContextView::updateContainmentsGeometry( bool updateAll )
{
    DEBUG_BLOCK
    
    const int width = rect().width();
    const int height = rect().height();
    
    if( m_zoomLevel == Plasma::DesktopZoom )
    {
        qreal left, top, right, bottom;
        containment()->getContentsMargins( &left, &top, &right, &bottom );
        if( updateAll )
        {
            foreach( Plasma::Containment *cont, contextScene()->containments() )
            {
                cont->resize( width + left + right, height + top + bottom );
                cont->updateConstraints();
            }
        }
        else
        {
            containment()->resize( width + left + right, height + top + bottom );
            containment()->updateConstraints();
            QRectF contRect( containment()->geometry() );
            setSceneRect( contRect.adjusted( left, top, -right, -bottom ) );
        }

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
ContextView::addContainment( const QVariantList& args )
{
    DEBUG_BLOCK

    Plasma::Corona* corona = containment()->corona();
    if (corona)
    {
        const int size = contextScene()->containments().size();
        Plasma::Containment *c = corona->addContainment( "context", args );
        c->setScreen( 0 );
        c->setFormFactor( Plasma::Planar );
        qreal left, top, right, bottom;
        containment()->getContentsMargins( &left, &top, &right, &bottom );
        QSizeF newSize( rect().width() + left + right, rect().height() + top + bottom );

        c->resize( newSize );
        c->updateConstraints();
        connectContainment( c );
        Containment *amarokContainment = qobject_cast< Containment * >( c );

        if( amarokContainment )
        {
            amarokContainment->setView( this );
            amarokContainment->setTitle( i18n( "Page #%1", size + 1 ) );
            amarokContainment->setFooter( QString::number( size + 1 ) );
        }
    }
}

void
ContextView::connectContainment( Plasma::Containment* containment )
{
    if( containment )
    {
        connect( containment, SIGNAL( zoomRequested( Plasma::Containment*, Plasma::ZoomDirection ) ),
                this, SLOT( zoom( Plasma::Containment*, Plasma::ZoomDirection ) ) );
        connect( containment, SIGNAL( zoomRequested( Plasma::Containment*, Plasma::ZoomDirection ) ),
                 this, SLOT( zoomIn( Plasma::Containment * ) ) );
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
        disconnect( containment, SIGNAL( zoomRequested( Plasma::Containment*, Plasma::ZoomDirection ) ),
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
ContextView::setContainment( Plasma::Containment* newContainment )
{
    DEBUG_BLOCK
    if( newContainment->isContainment() )
    {
        if( newContainment != containment() )
        {
            
            if( m_startupFinished )
            {
                //Resize the containment first because it probably had a wrong geometry.
                //Be aware that plasma automatically change all other containments position so they don't overlap.
                newContainment->resize( containment()->size() );
                newContainment->updateConstraints();

                //Plasma changes the containments placement but not inmediately so we can't rely on it yet
                const QList< Plasma::Containment* > containments = contextScene()->containments();
                int fromIndex = containments.indexOf( containment() );
                qreal width = containment()->size().width();
                qreal height = containment()->size().height();
                qreal x = width * ( fromIndex % CONTAINMENT_COLUMNS );
                qreal y = height * ( fromIndex / CONTAINMENT_COLUMNS );
                m_startPos = QRectF( QPointF( x, y ), QSizeF( width, height ) );

                if( m_zoomLevel == Plasma::DesktopZoom )
                {

                    int toIndex = containments.indexOf( newContainment );
                    x = width * ( toIndex % CONTAINMENT_COLUMNS );
                    y = height * ( toIndex / CONTAINMENT_COLUMNS );
                    m_destinationPos = QRectF( QPointF( x, y ), QSizeF( width, height ) );
                    
                    Containment* amarokContainment = qobject_cast<Containment*>( newContainment );
                    if( amarokContainment )
                        amarokContainment->setZoomLevel( Plasma::DesktopZoom );
                    
                    connect( Plasma::Animator::self(), SIGNAL( customAnimationFinished( int ) ),
                             this, SLOT( animateContainmentChangeFinished( int ) ) );
                             
                    Plasma::Animator::self()->customAnimation( m_startPos.width() / 30, 250,
                                                            Plasma::Animator::EaseInOutCurve,
                                                            this, "animateContainmentChange" );
                }
            }
            m_containment = newContainment;
        }
    }

}


void
ContextView::setContainment( Plasma::Containment* from, int direction ) // SLOT
{
    Q_UNUSED( from )
    DEBUG_BLOCK

    const QList< Plasma::Containment* > containments = contextScene()->containments();
    int fromIndex = containments.indexOf( containment() );    
    int size = containments.size();
    int newIndex = -1;

    switch( direction ) // NOTE this only works for 2x2 grid of containments
    {
    case UP:
    {
        newIndex = ( fromIndex - 2 ) % size;
        break;
    } case DOWN:
    {
        newIndex = ( fromIndex + 2 ) % size;
        break;
    } case UP_RIGHT: 
      case LEFT:
    {
        newIndex = ( fromIndex - 1 ) % size;
        break;
    } case DOWN_LEFT:
      case RIGHT:
    {
        newIndex = ( fromIndex + 1 ) % size;
        break;
    } case UP_LEFT:
    {
        newIndex = ( fromIndex - 3 ) % size;
        break;
    } case DOWN_RIGHT:
    {
        newIndex = ( fromIndex + 3 ) % size;
        break;
    } default:
        newIndex = 0;
    }
    if( newIndex < 0 )
        newIndex = 3; // for some reason here -1 % 4 != 3
    debug() << "switching to containment: " << newIndex;
    setContainment( containments.value( newIndex ) );
}
    


void
ContextView::nextContainment()
{
    DEBUG_BLOCK
    const QList<Plasma::Containment*> containments = contextScene()->containments();
    int index = containments.indexOf( containment() );
    index = ( index + 1 ) % containments.size();

    setContainment( containments.at( index ) );
}

void
ContextView::previousContainment()
{
    DEBUG_BLOCK
    const QList<Plasma::Containment*> containments = contextScene()->containments();
    int index = containments.indexOf( containment() );
    index--;
    if( index == -1 )
        index = contextScene()->containments().size() - 1; // last containment
        
    setContainment( containments.at( index ) );
}

void
ContextView::animateContainmentChange( qreal progress, int id )
{
    Q_UNUSED( id )
    DEBUG_BLOCK
    qreal incrementX;
    qreal incrementY;
    
    qreal left, top, right, bottom;
    
    containment()->getContentsMargins( &left, &top, &right, &bottom );
    
    qreal x = m_destinationPos.left() + left;    
    qreal y = m_destinationPos.top() + top;
        
    if( m_startPos.left() < m_destinationPos.left() )
    {
        incrementX = progress * ( m_destinationPos.width() + left + INTER_CONTAINMENT_MARGIN );
        x = m_startPos.left() + incrementX;
    }
    else if ( m_startPos.left() > m_destinationPos.left() )
    {
        incrementX = progress * ( m_destinationPos.width() );
        x = m_startPos.left() - incrementX;
    }
    if( m_startPos.top() < m_destinationPos.top() )
    {
        incrementY = progress * ( m_destinationPos.height() + top + INTER_CONTAINMENT_MARGIN );
        y = m_startPos.top() + incrementY;
    }
    else if( m_startPos.top() > m_destinationPos.top() )
    {
        incrementY = progress * ( m_destinationPos.height() );
        y = m_startPos.top() - incrementY;
    }
    debug() << "( "<<x << ", " << y << " ) ";
    QRectF visibleRect( QPointF( x, y ), m_destinationPos.size() );
    setSceneRect( visibleRect );
}

void
ContextView::animateContainmentChangeFinished( int id )
{
    Q_UNUSED( id )
    updateContainmentsGeometry( true );
    disconnect( Plasma::Animator::self(), SIGNAL( customAnimationFinished( int ) ),
                             this, SLOT( animateContainmentChangeFinished( int ) ) );
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

Plasma::Containment *
ContextView::containment()
{
    return m_containment;
}

} // Context namespace

#include "ContextView.moc"
