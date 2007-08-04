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
    , m_background( 0 )
    , m_bitmapBackground( 0 )
{
    DEBUG_BLOCK

    s_self = this;

    setFrameShape( QFrame::NoFrame );
    setAutoFillBackground( true );
    
    setScene( new ContextScene( rect(), this ) );
    scene()->setItemIndexMethod( QGraphicsScene::BspTreeIndex );
    //TODO: Figure out a way to use rubberband and ScrollHandDrag
    //setDragMode( QGraphicsView::RubberBandDrag );
    setTransformationAnchor( QGraphicsView::AnchorUnderMouse ); // Why isn't this working???
    setCacheMode( QGraphicsView::CacheBackground );
    setInteractive( true );
    setAcceptDrops( true );
    setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    setMouseTracking( true );
    
    // here we initialize all the Plasma paths to Amarok paths
    Theme::self()->setApplication( "amarok" );
    //contextScene()->setAppletMimeType( "text/x-amarokappletservicename" );
    
    //TODO: port to new config interface
    //KConfigGroup config(KGlobal::config(), "General");
    //m_wallpaperPath = config.readEntry("wallpaper", QString());
    m_wallpaperPath = QString();
    
    //kDebug() << "wallpaperPath is " << m_
    if ( m_wallpaperPath.isEmpty() ||
        !QFile::exists(m_wallpaperPath ) )
        m_background = new Svg( "widgets/amarok-wallpaper", this );
    
    init();
    showHome();
}

ContextView::~ContextView() 
{
    DEBUG_BLOCK
    clear( m_curState );
}


void ContextView::init()
{
    DEBUG_BLOCK
    m_padding = 3;
    m_defaultColumnSize = 400;
    qreal width = sceneRect().width();
    int numColumns = (int)( width - ( 2 * m_padding ) ) / m_defaultColumnSize;
    if( numColumns == 0 ) numColumns = 1;
    debug() << "need to create:" << numColumns << "columns";
    for( int i = 0; i < numColumns; i++ )
        m_columns << new Plasma::VBoxLayout();
    resizeColumns();
}

void ContextView::resizeColumns()
{
    DEBUG_BLOCK
    qreal width = sceneRect().width() - 2 * m_padding;
    int numColumns = ( width - 2 * m_padding ) / m_defaultColumnSize;
    if( numColumns > m_columns.size() ) // need to make more columns
    {
        for( int i = m_columns.size(); i < numColumns; i++ )
            m_columns << new Plasma::VBoxLayout();
    }
    
    qreal columnWidth = width / numColumns;
    columnWidth -= ( numColumns - 1 ) * m_padding; // make room between columns
    
    for( int i = 0; i < numColumns; i++ ) // lay out columns
    {
        QPointF pos( ( ( i + 1 ) * m_padding ) + ( i * columnWidth ), m_padding );
        QSizeF size( columnWidth, qMax( m_columns[ i ]->minimumSize().height(),
                                        sceneRect().height() ) );
        m_columns[ i ]->setGeometry( QRectF( pos, size ) );
    }
    balanceColumns();
}

// even out columns. this checks if any one column can be made shorter by
// moving the last applet to another column
void ContextView::balanceColumns()
{
    DEBUG_BLOCK
    int numColumns = m_columns.size();
    if( numColumns == 1 ) // no balancing to do :)
        return;

    while( 0 )
    {
        qreal maxHeight  = -1; int maxColumn = -1;
        for( int i = 0; i < numColumns; i++ )
        {
            if( m_columns[ i ]->size().height() > maxHeight )
            {
                maxHeight = m_columns[ i ]->size().height();
                maxColumn = i;
            }
        }
        
        if( maxHeight == 0 ) // no applets
            return;
        
        debug() << "found maxHeight:" << maxHeight << "and maxColumn:" << maxColumn;
        
        qreal maxAppletHeight = m_columns[ maxColumn ]->itemAt( m_columns[ maxColumn ]->count() - 1 )->geometry().size().height();
        
        debug() << "found maxHeight:" << maxHeight << "and maxColumn:" << maxColumn << "and maxAppletHeight" << maxAppletHeight;
        int newHeight = -1, newColumn = -1;
        bool found = false;
        for( int i = 0; i < numColumns; i++ )
        {
            qreal newColHeight = m_columns[ i ]->size().height() + newHeight;
            debug() << "checking if newColHeight:" << newColHeight << "is less than:" << maxHeight;
            if( newColHeight < maxHeight ) // found a new place for this applet
            {
                debug() << "found new place for an applet!";
                m_columns[ i ]->addItem( m_columns[ maxColumn ]->takeAt( m_columns[ maxColumn ]->count() - 1 ) );
                found = true;
                break;
            }
        }
        if( !found ) break;
    }
}

void ContextView::clear()
{    
    DEBUG_BLOCK;
    m_columns.clear();
}

void ContextView::clear( const ContextState& state )
{
    QString name;
    if( state == Home )
        name = "home";
    else if( state == Current )
        name = "current";
    else
        return; // startup, or some other wierd case
    
    QStringList applets;
    foreach( Plasma::VBoxLayout* column, m_columns )
    {
        for( int i = 0; i < column->count() - 1; i++ )
        {
            Applet* applet = dynamic_cast< Applet* >( column->itemAt( i ) );
            if( applet == 0 ) continue;
            QString key = QString( "%1_%2" ).arg( name, applet->name() );
            applets << applet->name();
            QStringList pos;
            pos << QString::number( applet->x() ) << QString::number( applet->y() );
            Amarok::config( "Context Applets" ).writeEntry( key, pos );
            debug() << "saved applet: " << key << " at position: " << pos;
        }
    }
    debug() << "saved list of applets: " << applets;
    Amarok::config( "Context Applets" ).writeEntry( name, applets );
    Amarok::config( "Context Applets" ).sync();
    m_columns.clear();
}


void ContextView::engineStateChanged( Engine::State state, Engine::State oldState )
{
    DEBUG_BLOCK
    Q_UNUSED( oldState );
    
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
        //clear( m_curState );
    m_curState = Home;
    loadConfig();
    messageNotify( m_curState );
}

void ContextView::showCurrentTrack()
{
    DEBUG_BLOCK
        //clear( m_curState );
    m_curState = Current;
    loadConfig();
    messageNotify( Current );
}

// loads applets onto the ContextScene from saved data, using m_curState
void ContextView::loadConfig()
{
    DEBUG_BLOCK
    QString cur;
    if( m_curState == Home ) 
        cur == QString( "home" );
    else if( m_curState == Current )
        cur == QString( "current" );
    
    QStringList applets = Amarok::config( "Context Applets" ).readEntry( cur, QStringList() );
    foreach( QString applet, applets )
    {
        QString key = QString( "%1_%2" ).arg( cur, applet );
        QStringList pos = Amarok::config( "Context Applets" ).readEntry( key, QStringList() );
        debug() << "trying to restore: " << key << " at: " << pos;
        QString constraint = QString( "[Name] == '%1'" ).arg( applet );
        KService::List offers = KServiceTypeTrader::self()->query( "Plasma/Applet", constraint ); // find the right one
        KPluginInfo::List plugins = KPluginInfo::fromServices( offers );
        if( plugins.size() > 0 )
            contextScene()->addApplet( plugins[0].pluginName(), pos ); // for now we only load the first result (there should only be one...)
        else
            warning() << "Help! tried to load a non-existent plugin: " << applet << " at: " << pos << endl;
    }
    Amarok::config( "Context Applets" ).sync();
}

void ContextView::engineNewMetaData( const MetaBundle&, bool )
{
    ; //clear();
}


Applet* ContextView::addApplet(const QString& name, const QStringList& args)
{
    AppletPointer applet = contextScene()->addApplet( name, args );
    
    int smallestColumn = -1, max = -1;
    for( int i = 0; i < m_columns.size(); i++ ) // find shortest column to put
    {                                           // the applet in
        if( m_columns[ i ]->size().height() > max )
            smallestColumn = i;
    }
    
    debug() << "found" << m_columns.size() << " column, adding applet to column:" << smallestColumn;
    m_columns[ smallestColumn ]->addItem( applet );
    return applet;
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

void ContextView::drawBackground( QPainter * painter, const QRectF & rect )
{
    if ( m_background ) {
        m_background->paint( painter, rect );
    } else if ( m_bitmapBackground ) {
        painter->drawPixmap( rect, *m_bitmapBackground, rect );
    }
}

void ContextView::resizeEvent( QResizeEvent* event )
{
    Q_UNUSED( event )
        if ( testAttribute( Qt::WA_PendingResizeEvent ) ) {
            return; // lets not do this more than necessary, shall we?
        }
    
    scene()->setSceneRect( rect() );
    
    if ( m_background ) {
        m_background->resize( width(), height() );
    } else if ( !m_wallpaperPath.isEmpty() ) {
        delete m_bitmapBackground;
        m_bitmapBackground = new QPixmap( m_wallpaperPath );
        ( *m_bitmapBackground ) = m_bitmapBackground->scaled( size() );
    }
    
    resizeColumns();
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

void ContextView::contextMenuEvent(QContextMenuEvent *event)
{
    if ( !scene() ) {
        QGraphicsView::contextMenuEvent( event );
        return;
    }
    
    QPointF point = event->pos();
    QPointF globalPoint = event->globalPos();

    QGraphicsItem* item = scene()->itemAt(point);
    Plasma::Applet* applet = 0;
    
    while (item) {
        applet = qgraphicsitem_cast<Plasma::Applet*>(item);
        if (applet) {
            break;
        }
        
        item = item->parentItem();
    }
    
    KMenu desktopMenu;
    //kDebug() << "context menu event " << immutable << endl;
    if (!applet) {
        if (contextScene() && contextScene()->isImmutable()) {
            QGraphicsView::contextMenuEvent(event);
            return;
        }
        
                //FIXME: change this to show this only in debug mode (or not at all?)
        //       before final release
        
    } else if (applet->isImmutable()) {
        QGraphicsView::contextMenuEvent(event);
        return;
    } else {
        //desktopMenu.addSeparator();
        bool hasEntries = false;
        if (applet->hasConfigurationInterface()) {
            QAction* configureApplet = new QAction(i18n("%1 Settings...", applet->name()), this);
            connect(configureApplet, SIGNAL(triggered(bool)),
                    applet, SLOT(showConfigurationInterface()));
            desktopMenu.addAction(configureApplet);
            hasEntries = true;
        }
        
        if (!contextScene() || !contextScene()->isImmutable()) {
            QAction* closeApplet = new QAction(i18n("Close this %1", applet->name()), this);
            connect(closeApplet, SIGNAL(triggered(bool)),
                    applet, SLOT(deleteLater()));
            desktopMenu.addAction(closeApplet);
            hasEntries = true;
        }
        
        if (!hasEntries) {
            QGraphicsView::contextMenuEvent(event);
            return;
        }
    }
    
    event->accept();
    desktopMenu.exec(globalPoint.toPoint());
}


} // Context namespace

#include "ContextView.moc"
