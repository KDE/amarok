/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "debug.h"
#include "meta/MetaUtility.h"
#include "PlaylistGraphicsItem.h"
#include "PlaylistModel.h"
#include "TheInstances.h"

#include <QBrush>
#include <QFontMetricsF>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QPen>
#include <QRadialGradient>
#include <QScrollBar>
#include <QStyleOptionGraphicsItem>

struct PlaylistNS::GraphicsItem::ActiveItems
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

const qreal PlaylistNS::GraphicsItem::ALBUM_WIDTH = 50.0;
const qreal PlaylistNS::GraphicsItem::MARGIN = 2.0;
qreal PlaylistNS::GraphicsItem::s_height = -1.0;
QFontMetricsF* PlaylistNS::GraphicsItem::s_fm = 0;

PlaylistNS::GraphicsItem::GraphicsItem()
    : QGraphicsItem()
    , m_items( 0 )
{
    if( not s_fm )
    {
        s_fm = new QFontMetricsF( QFont() );
        s_height =  qMax( ALBUM_WIDTH, s_fm->height() * 2 );
    }
  //  setHandlesChildEvents( true );
    setFlag( QGraphicsItem::ItemIsSelectable );
}

PlaylistNS::GraphicsItem::~GraphicsItem()
{
    delete m_items;
}

void 
PlaylistNS::GraphicsItem::paint( QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget )
{
    const int row = int( ( mapToScene( 0.0, 0.0 ).y() ) / s_height );
    const QModelIndex index = The::playlistModel()->index( row, 0 );
    if( not m_items || ( option->rect.width() != m_items->lastWidth ) )
    {

        if( not m_items )
        {
            const Meta::TrackPtr track = index.data( ItemRole ).value< PlaylistNS::Item* >()->track();
            m_items = new PlaylistNS::GraphicsItem::ActiveItems();
            m_items->track = track;
            init( track );
        }
        resize( m_items->track, option->rect.width() );
    }
    if( option->state & QStyle::State_Selected )
    {
        if( not m_items->background )
        {
            m_items->background = new QGraphicsRectItem( option->rect, this );
            m_items->background->setPos( 0.0, 0.0 );
            m_items->background->setPen( QPen( Qt::NoPen ) );
            m_items->background->setBrush( option->palette.highlight() );
            m_items->background->setZValue( -5.0 );
        }
        else
            m_items->background->show();
    }
    else if( m_items->background )
        m_items->background->hide();

    if( index.data( ActiveTrackRole ).toBool() )
    {
        if( not m_items->foreground )
        {
            m_items->foreground = new QGraphicsRectItem( option->rect, this );
            m_items->foreground->setPos( 0.0, 0.0 );
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
            m_items->foreground->show();
    }
    else if( m_items->foreground )
        m_items->foreground->hide();
}

void
PlaylistNS::GraphicsItem::init( Meta::TrackPtr track )
{
    QPixmap albumPixmap;
    if( track->album() )
        albumPixmap =  track->album()->image( int( ALBUM_WIDTH ) );

    m_items->albumArt = new QGraphicsPixmapItem( albumPixmap, this );
    m_items->albumArt->setPos( 0.0, 0.0 );

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
PlaylistNS::GraphicsItem::resize( Meta::TrackPtr track, int totalWidth )
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

    const qreal lineTwoY = s_height / 2;
    const qreal textWidth = ( ( qreal( totalWidth ) - ALBUM_WIDTH ) / 2.0 );
    const qreal leftAlignX = ALBUM_WIDTH + MARGIN;
    qreal rightAlignX;
    {
        qreal middle = textWidth + ALBUM_WIDTH + ( MARGIN * 2.0 );
        qreal rightWidth = totalWidth - qMax( s_fm->width( album )
            , s_fm->width( prettyLength ) );
        rightAlignX = qMax( middle, rightWidth );
    }
    m_items->topRightText->setPos( rightAlignX, 0.0 );
    m_items->bottomRightText->setPos( rightAlignX, lineTwoY );
    m_items->topRightText->setPlainText( s_fm->elidedText( album, Qt::ElideRight, totalWidth - rightAlignX ) );
    m_items->bottomRightText->setPlainText( s_fm->elidedText( prettyLength, Qt::ElideRight, totalWidth - rightAlignX ) );


    qreal spaceForLeft = totalWidth - ( totalWidth - rightAlignX ) - leftAlignX;
    {
        QString artist;
        if( track->artist() )
            artist = track->artist()->name();
        m_items->topLeftText->setPlainText( s_fm->elidedText( artist, Qt::ElideRight, spaceForLeft ) );
        m_items->topLeftText->setPos( leftAlignX, 0.0 );
    }

    m_items->bottomLeftText->setPlainText( s_fm->elidedText( QString("%1 - %2").arg( QString::number( track->trackNumber() ), track->name() )
        , Qt::ElideRight, spaceForLeft ) );
    m_items->bottomLeftText->setPos( leftAlignX, lineTwoY );
}

void 
PlaylistNS::GraphicsItem::mouseDoubleClickEvent( QGraphicsSceneMouseEvent* event )
{
    if( m_items )
    {
        The::playlistModel()->play( ( mapToScene( 0.0, 0.0 ).y() ) / s_height );
        event->accept();
        return;
    }
    QGraphicsItem::mouseDoubleClickEvent( event );
}

QRectF
PlaylistNS::GraphicsItem::boundingRect() const
{
    const static int scrollBarWidth = scene()->views().at(0)->verticalScrollBar()->width();
    return QRectF( 0.0, 0.0, scene()->views().at(0)->width() - scrollBarWidth, s_height );
}
