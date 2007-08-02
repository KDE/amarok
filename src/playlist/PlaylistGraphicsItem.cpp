/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "debug.h"
#include "metabundle.h"
#include "PlaylistGraphicsItem.h"
#include "PlaylistModel.h"
#include "TheInstances.h"

#include <QFontMetricsF>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsView>
#include <QScrollBar>
#include <QStyleOptionGraphicsItem>

struct PlaylistNS::GraphicsItem::ActiveItems
{
    ActiveItems()
    : topLeftText( 0 )
    , bottomLeftText( 0 )
    , topRightText( 0 )
    , bottomRightText( 0 )
    , lastWidth( -5 )
    { }
    ~ActiveItems()
    {
        delete topLeftText;
        delete bottomLeftText;
        delete topRightText;
        delete bottomRightText;
    }
    QGraphicsPixmapItem* albumArt;
    QGraphicsTextItem* topLeftText;
    QGraphicsTextItem* bottomLeftText;
    QGraphicsTextItem* topRightText;
    QGraphicsTextItem* bottomRightText;
    int lastWidth;
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

}

PlaylistNS::GraphicsItem::~GraphicsItem()
{
    delete m_items;
}

void 
PlaylistNS::GraphicsItem::paint( QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget )
{
    if( not m_items || ( option->rect.width() != m_items->lastWidth ) )
    {
        const int row = ( mapToScene( 0.0, 0.0 ).y() ) / s_height;
        const Meta::TrackPtr track = The::playlistModel()->index( row, 0 ).data( ItemRole ).value< PlaylistNS::Item* >()->track();
        if( not m_items )
        {
            m_items = new PlaylistNS::GraphicsItem::ActiveItems();
            init( track );
        }
        resize( track, option->rect.width() );
    }
  /*  QGraphicsItem *items[5] = { albumArt, topLeftText, bottomLeftText, topRightText, bottomRightText };
    QStyleOptionGraphicsItem *options[5] = { option, option, option, option, option };
    scene->drawItems( painter, */
//    scene()->invalidate( option->rect, QGraphicsScene::ItemLayer );
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
    QString prettyLength = MetaBundle::prettyTime( track->length(), false );
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

QRectF
PlaylistNS::GraphicsItem::boundingRect() const
{
    const static int scrollBarWidth = scene()->views().at(0)->verticalScrollBar()->width();
    return QRectF( 0.0, 0.0, scene()->views().at(0)->width() - scrollBarWidth, s_height );
}
