/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "debug.h"
#include "meta/MetaUtility.h"
#include "AmarokMimeData.h"
#include "PlaylistGraphicsItem.h"
#include "PlaylistDropVis.h"
#include "PlaylistModel.h"
#include "TheInstances.h"

#include <QBrush>
#include <QDrag>
#include <QFontMetricsF>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QMimeData>
#include <QPen>
#include <QRadialGradient>
#include <QScrollBar>
#include <QStyleOptionGraphicsItem>

struct Playlist::GraphicsItem::ActiveItems
{
    ActiveItems()
    : topLeftText( 0 )
    , bottomLeftText( 0 )
    , topRightText( 0 )
    , bottomRightText( 0 )
    , background( 0 )
    , foreground( 0 )
    , lastWidth( -5 )
    { }
    ~ActiveItems()
    {
        delete albumArt;
        delete topLeftText;
        delete bottomLeftText;
        delete topRightText;
        delete bottomRightText;
        delete background;
        delete foreground;
    }
    QGraphicsPixmapItem* albumArt;
    QGraphicsTextItem* topLeftText;
    QGraphicsTextItem* bottomLeftText;
    QGraphicsTextItem* topRightText;
    QGraphicsTextItem* bottomRightText;
    QGraphicsRectItem* background;
    QGraphicsRectItem* foreground;
    int lastWidth;
    Meta::TrackPtr track;
};

const qreal Playlist::GraphicsItem::ALBUM_WIDTH = 50.0;
const qreal Playlist::GraphicsItem::MARGIN = 2.0;
qreal Playlist::GraphicsItem::s_height = -1.0;
QFontMetricsF* Playlist::GraphicsItem::s_fm = 0;

Playlist::GraphicsItem::GraphicsItem()
    : QGraphicsItem()
    , m_items( 0 )
    , m_track( 0 )
    , m_verticalOffset( 2.0 )
{
    if( not s_fm )
    {
        s_fm = new QFontMetricsF( QFont() );
        s_height =  qMax( ALBUM_WIDTH, s_fm->height() * 2 ) + 2 * m_verticalOffset;
    }
    setFlag( QGraphicsItem::ItemIsSelectable );
    setFlag( QGraphicsItem::ItemIsMovable );
    setAcceptDrops( true );
}

Playlist::GraphicsItem::~GraphicsItem()
{
    delete m_items;
}

void 
Playlist::GraphicsItem::paint( QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget )
{
    Q_UNUSED( painter ); Q_UNUSED( widget );
    const int row = getRow();
    const QModelIndex index = The::playlistModel()->index( row, 0 );

    if( not m_items || ( option->rect.width() != m_items->lastWidth ) )
    {

        if( not m_items )
        {
            const Meta::TrackPtr track = index.data( ItemRole ).value< Playlist::Item* >()->track();
            m_items = new Playlist::GraphicsItem::ActiveItems();
            m_items->track = track;
            init( track );
        }
        resize( m_items->track, option->rect.width() );
    }

    if( not m_items->background )
    {
	m_items->background = new QGraphicsRectItem( option->rect, this );
	m_items->background->setPos( 0.0, 0.0 );
	m_items->background->setPen( QPen( Qt::NoPen ) );
	m_items->background->setBrush( option->palette.highlight() );
	m_items->background->setZValue( -5.0 );
    }


    if( option->state & QStyle::State_Selected )
    {
        m_items->background->setBrush( option->palette.highlight() );
        m_items->background->show();
    }
    else  
    {
       //alternate color of background
       if ( row % 2 )
           m_items->background->setBrush( option->palette.base() );
       else
           m_items->background->setBrush( option->palette.alternateBase() );

        m_items->background->show();
    }

    if( index.data( ActiveTrackRole ).toBool() )
    {
        if( not m_items->foreground )
        {
            m_items->foreground = new QGraphicsRectItem( option->rect, this );
            m_items->foreground->setPos( 0.0, m_verticalOffset );
            m_items->foreground->setZValue( 5.0 );
            QRadialGradient gradient(option->rect.width() / 2.0, option->rect.height() / 2.0, option->rect.width() / 2.0, 20 + option->rect.width() / 2.0, option->rect.height() / 2.0 );
            QColor start = option->palette.highlight().color().light();
            start.setAlpha( 51 );
            QColor end = option->palette.highlight().color().dark();
            end.setAlpha( 51 );
            gradient.setColorAt( 0.0, start );
            gradient.setColorAt( 1.0, end );
            QBrush brush( gradient );
            m_items->foreground->setBrush( brush );
            m_items->foreground->setPen( QPen( Qt::NoPen ) );
        }
        else
            m_items->background->hide();
            m_items->foreground->show();
    }
    else if( m_items->foreground )
        m_items->foreground->hide();

    m_items->albumArt->show();
}

void
Playlist::GraphicsItem::init( Meta::TrackPtr track )
{
    m_track = track;

    setHandlesChildEvents( true ); // don't let drops etc hit the text items, doing stupid things

    QPixmap albumPixmap;
    if( track->album() )
        albumPixmap =  track->album()->image( int( ALBUM_WIDTH ) );

    m_items->albumArt = new QGraphicsPixmapItem( albumPixmap, this );
    m_items->albumArt->setPos( 0.0, m_verticalOffset );

    {
        QFont font;
        font.setPointSize( font.pointSize() - 1 );
        #define NewText( X ) \
            X = new QGraphicsTextItem( this ); \
            X->setTextInteractionFlags( Qt::TextEditorInteraction ); \
            X->setFont( font );
        NewText( m_items->topLeftText )       
        NewText( m_items->bottomLeftText )
        NewText( m_items->topRightText )
        NewText( m_items->bottomRightText )
        #undef NewText
    }
}

void
Playlist::GraphicsItem::resize( Meta::TrackPtr track, int totalWidth )
{
    if( totalWidth == -1 || totalWidth == m_items->lastWidth ) //no change needed
        return;
    if( m_items->lastWidth != -5 ) //this isn't the first "resize"
        prepareGeometryChange();
    m_items->lastWidth = totalWidth;
    QString prettyLength = Meta::secToPrettyTime( track->length() );
    QString album;
    if( track->album() )
        album = track->album()->name();

    const qreal lineTwoY = s_height / 2 + m_verticalOffset;
    const qreal textWidth = ( ( qreal( totalWidth ) - ALBUM_WIDTH ) / 2.0 );
    const qreal leftAlignX = ALBUM_WIDTH + MARGIN;
    qreal rightAlignX;
    {
        qreal middle = textWidth + ALBUM_WIDTH + ( MARGIN * 2.0 );
        qreal rightWidth = totalWidth - qMax( s_fm->width( album )
            , s_fm->width( prettyLength ) );
        rightAlignX = qMax( middle, rightWidth );
    }
    m_items->topRightText->setPos( rightAlignX, m_verticalOffset );
    m_items->bottomRightText->setPos( rightAlignX, lineTwoY );
    m_items->topRightText->setPlainText( s_fm->elidedText( album, Qt::ElideRight, totalWidth - rightAlignX ) );
    m_items->bottomRightText->setPlainText( s_fm->elidedText( prettyLength, Qt::ElideRight, totalWidth - rightAlignX ) );


    qreal spaceForLeft = totalWidth - ( totalWidth - rightAlignX ) - leftAlignX;
    {
        QString artist;
        if( track->artist() )
            artist = track->artist()->name();
        m_items->topLeftText->setPlainText( s_fm->elidedText( artist, Qt::ElideRight, spaceForLeft ) );
        m_items->topLeftText->setPos( leftAlignX, m_verticalOffset );
    }

    m_items->bottomLeftText->setPlainText( s_fm->elidedText( QString("%1 - %2").arg( QString::number( track->trackNumber() ), track->name() )
        , Qt::ElideRight, spaceForLeft ) );
    m_items->bottomLeftText->setPos( leftAlignX, lineTwoY );
}

QRectF
Playlist::GraphicsItem::boundingRect() const
{
    // the viewport()->size() takes scrollbars into account
    return QRectF( 0.0, 0.0, scene()->views().at(0)->viewport()->size().width(), s_height );
}

void
Playlist::GraphicsItem::play()
{
    The::playlistModel()->play( getRow() );
}

void 
Playlist::GraphicsItem::mouseDoubleClickEvent( QGraphicsSceneMouseEvent *event )
{
    if( m_items )
    {
        event->accept();
        play();
        return;
    }
    QGraphicsItem::mouseDoubleClickEvent( event );
}

void
Playlist::GraphicsItem::mousePressEvent( QGraphicsSceneMouseEvent *event )
{
    debug() << event->pos();
    if( event->buttons() & Qt::RightButton )
    {
        event->ignore();
        return;
    }
    QGraphicsItem::mousePressEvent( event );
    /*
    AmarokMimeData *mime= new AmarokMimeData();
    Meta::TrackList tracks;
    tracks << m_track;
    mime->setTracks( tracks );

    QDrag *drag = new QDrag( event->widget() );
    drag->setMimeData( mime );

    drag->start();
    */
}

// With help from QGraphicsView::mouseMoveEvent()
void
Playlist::GraphicsItem::mouseMoveEvent( QGraphicsSceneMouseEvent *event )
{
    if( (event->buttons() & Qt::LeftButton) && ( flags() & QGraphicsItem::ItemIsMovable))
    {
        // Determine the list of selected items
        QList<QGraphicsItem *> selectedItems = scene()->selectedItems();
        if( !isSelected() )
            selectedItems << this;
        // Move all selected items
        foreach( QGraphicsItem *item, selectedItems )
        {
            if( (item->flags() & QGraphicsItem::ItemIsMovable) && (!item->parentItem() || !item->parentItem()->isSelected()) )
            {
                QPointF diff;
                if( item == this )
                {
                    diff = mapToParent(event->pos()) - mapToParent(event->lastPos());
                }
                else
                {
                    diff = item->mapToParent( item->mapFromScene(event->scenePos()))
                                              - item->mapToParent(item->mapFromScene(event->lastScenePos()));
                }

                item->moveBy( 0, diff.y() );
                if( item->flags() & ItemIsSelectable )
                    item->setSelected( true );
            }
        }
    }
    else
    {
        QGraphicsItem::mouseMoveEvent( event );
    }
}

void 
Playlist::GraphicsItem::dragEnterEvent( QGraphicsSceneDragDropEvent *event )
{
    foreach( QString mime, The::playlistModel()->mimeTypes() )
    {
        if( event->mimeData()->hasFormat( mime ) )
        {
            event->accept();
            Playlist::DropVis::instance()->showDropIndicator( this );
            break;
        }
    }
}

void
Playlist::GraphicsItem::dropEvent( QGraphicsSceneDragDropEvent * event )
{
    Qt::DropAction dropAction = Qt::CopyAction;
    if( event->source() == scene()->views().at(0) )
    {
        debug() << "internal drop!";
        dropAction = Qt::MoveAction;
    }

    event->accept();
    The::playlistModel()->dropMimeData( event->mimeData(), dropAction, getRow(), 0, QModelIndex() );
    Playlist::DropVis::instance()->hide();
}

void 
Playlist::GraphicsItem::refresh()
{
    QPixmap albumPixmap;
    if( !m_track )
        return;

    if( m_track->album() )
        albumPixmap =  m_track->album()->image( int( ALBUM_WIDTH ) );

    m_items->albumArt->hide();
    delete ( m_items->albumArt );
    m_items->albumArt = new QGraphicsPixmapItem( albumPixmap, this );
    m_items->albumArt->setPos( 0.0, m_verticalOffset );
}
