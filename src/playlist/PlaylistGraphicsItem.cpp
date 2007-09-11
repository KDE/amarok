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
#include "PlaylistGraphicsView.h"
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
    : albumArt( 0 )
    , background( 0 )
    , bottomLeftText( 0 )
    , bottomRightText( 0 )
    , foreground( 0 )
    , lastWidth( -5 )
    , topLeftText( 0 )
    , topRightText( 0 )
    { }
    ~ActiveItems()
    {
        delete albumArt;
        delete background;
        delete bottomLeftText;
        delete bottomRightText;
        delete foreground;
        delete topLeftText;
        delete topRightText;
     }
    QGraphicsPixmapItem* albumArt;
    QGraphicsRectItem* background;
    QGraphicsRectItem* foreground;
    QGraphicsTextItem* bottomLeftText;
    QGraphicsTextItem* bottomRightText;
    QGraphicsTextItem* topLeftText;
    QGraphicsTextItem* topRightText;
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
    setZValue( 1.0 );
    if( !s_fm )
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
// ::paint RULES:
// 1) You do not talk about ::paint method
// 2) You DO NOT talk about ::paint method
// 3) Do not show or hide item that are already shown or hidden, respectively
// 4) Do not setBrush without making sure its hasn't already been set to that brush().
// 5) If this is your first night at ::paint method, you HAVE to paint.
    DEBUG_BLOCK
    Q_UNUSED( painter ); Q_UNUSED( widget );
    const int row = getRow();
    const QModelIndex index = The::playlistModel()->index( row, 0 );

    if( !m_items || ( option->rect.width() != m_items->lastWidth ) )
    {

        if( !m_items )
        {
            const Meta::TrackPtr track = index.data( ItemRole ).value< Playlist::Item* >()->track();
            m_items = new Playlist::GraphicsItem::ActiveItems();
            m_items->track = track;
            init( track );
        }
        resize( m_items->track, option->rect.width() );
    }

    { //background mania. blue if selected, alternating white/gray otherwise.
        #define SetBackgroundBrush( B ) \
            if( m_items->background->brush() != B ) \
                m_items->background->setBrush( B );
        if( !m_items->background )
        {
                m_items->background = new QGraphicsRectItem( option->rect, this );
                m_items->background->setPos( 0.0, 0.0 );
                m_items->background->setPen( QPen( Qt::NoPen ) );
                m_items->background->setZValue( -5.0 );
                m_items->background->show();
        }
        if( option->state & QStyle::State_Selected )
        {
            SetBackgroundBrush( option->palette.highlight() );
        }
        else if( row % 2 )
        {
            SetBackgroundBrush( option->palette.base()  );
        }
        else
        {
            SetBackgroundBrush( option->palette.alternateBase() );
        }
        #undef SetBrush
    }

    if( index.data( ActiveTrackRole ).toBool() )
    {
        if( !m_items->foreground )
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
        if( !m_items->foreground->isVisible() ) 
            m_items->foreground->show();
    }
    else if( m_items->foreground && m_items->foreground->isVisible() )
        m_items->foreground->hide();
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

    m_items->lastWidth = totalWidth;
}

QRectF
Playlist::GraphicsItem::boundingRect() const
{
    // the viewport()->size() takes scrollbars into account
    return QRectF( 0.0, 0.0, The::playlistView()->viewport()->size().width(), s_height );
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
    if( event->buttons() & Qt::RightButton )
    {
        event->ignore();
        return;
    }
    m_preDragLocation = mapToScene( boundingRect() ).boundingRect();
    QGraphicsItem::mousePressEvent( event );
}

// With help from QGraphicsView::mouseMoveEvent()
void
Playlist::GraphicsItem::mouseMoveEvent( QGraphicsSceneMouseEvent *event )
{
    if( (event->buttons() & Qt::LeftButton) && ( flags() & QGraphicsItem::ItemIsMovable))
    {
        bool dragOverOriginalPosition = m_preDragLocation.contains( event->scenePos() );
        debug() << "originalLocation (" << m_preDragLocation << ") contains currentPoint (" << event->scenePos() << ") ? " << dragOverOriginalPosition;

        //make sure item is drawn on top of other items
        setZValue( 2.0 );

        // Determine the list of selected items
        QList<QGraphicsItem *> selectedItems = scene()->selectedItems();
        if( !isSelected() )
            selectedItems << this;
        // Move all selected items
        foreach( QGraphicsItem *item, selectedItems )
        {
            if( (item->flags() & QGraphicsItem::ItemIsMovable) && (!item->parentItem() || !item->parentItem()->isSelected()) )
            {
                Playlist::GraphicsItem *above = 0;
                QPointF diff;
                if( item == this && !dragOverOriginalPosition )
                {
                    diff = event->scenePos() - event->lastScenePos();
                    QList<QGraphicsItem*> collisions = scene()->items( event->scenePos() );
                    foreach( QGraphicsItem *i, collisions )
                    {
                        Playlist::GraphicsItem *c = dynamic_cast<Playlist::GraphicsItem *>( i );
                        if( c && c != this )
                        {
                            above = c;
                            break;
                        }
                    }
                }
                else
                {
                    diff = item->mapToParent( item->mapFromScene(event->scenePos()))
                                              - item->mapToParent(item->mapFromScene(event->lastScenePos()));
                }

                item->moveBy( 0, diff.y() );
                if( item->flags() & ItemIsSelectable )
                    item->setSelected( true );
                
                if( dragOverOriginalPosition )
                    Playlist::DropVis::instance()->showDropIndicator( m_preDragLocation.y() );
                else
                    Playlist::DropVis::instance()->showDropIndicator( above );
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
    event->accept();
    setZValue( 1.0 );
    The::playlistModel()->dropMimeData( event->mimeData(), Qt::CopyAction, getRow(), 0, QModelIndex() );
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

void Playlist::GraphicsItem::mouseReleaseEvent( QGraphicsSceneMouseEvent *event )
{
    bool dragOverOriginalPosition = m_preDragLocation.contains( event->scenePos() );
    if( dragOverOriginalPosition )
    {
        setPos( m_preDragLocation.topLeft() );
        Playlist::DropVis::instance()->hide();
        return;
    }

    Playlist::GraphicsItem *above = 0;
    QList<QGraphicsItem*> collisions = scene()->items( event->scenePos() );
    foreach( QGraphicsItem *i, collisions )
    {
        Playlist::GraphicsItem *c = dynamic_cast<Playlist::GraphicsItem *>( i );
        if( c && c != this )
        {
            above = c;
            break;
        }
    }
    // if we've dropped ourself ontop of another item, then we need to shuffle the tracks below down
    if( above )
    {
        setPos( above->pos() );
        The::playlistView()->moveItem( this, above );
    }

    //make sure item resets its z value
    setZValue( 1.0 );
    Playlist::DropVis::instance()->hide();
}
