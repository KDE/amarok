/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "ItemScene.h"
#include "metabundle.h" //just for prettyTime
#include "PlaylistItem.h"


#include <QFontMetricsF>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QList>

const qreal PlaylistNS::ItemScene::ALBUM_WIDTH = 50.0;
const qreal PlaylistNS::ItemScene::MARGIN = 2.0;
qreal PlaylistNS::ItemScene::s_height = -1.0;
QFontMetricsF* PlaylistNS::ItemScene::s_fm = 0;

PlaylistNS::ItemScene::ItemScene( Meta::TrackPtr track )
    : QGraphicsScene()
    , m_track( track )
    , m_topLeftText( 0 )
    , m_bottomLeftText( 0 )
    , m_topRightText( 0 )
    , m_bottomRightText( 0 )
{
    if( not s_fm )
    {
        s_fm = new QFontMetricsF( QFont() );
        s_height =  qMax( ALBUM_WIDTH, s_fm->height() * 2 );
    }

    QPixmap albumPixmap;
    if( m_track->album() )
        albumPixmap =  m_track->album()->image( int( ALBUM_WIDTH ) );

    QGraphicsPixmapItem* pixmap = new QGraphicsPixmapItem( albumPixmap );
    pixmap->setPos( 0.0, 0.0 );

    {
        QFont font;
        font.setPointSize( font.pointSize() - 1 );
        #define NewText( X ) \
            X = new QGraphicsTextItem(); \
            X->setTextInteractionFlags( Qt::TextEditorInteraction ); \
            X->setFont( font );
        NewText( m_topLeftText )       
        NewText( m_bottomLeftText )
        NewText( m_topRightText )
        NewText( m_bottomRightText )
        #undef NewText
    }
    addItem( pixmap );
    addItem( m_topLeftText );
    addItem( m_bottomLeftText );
    addItem( m_topRightText );
    addItem( m_bottomRightText );
}

void
PlaylistNS::ItemScene::resize( int totalWidth )
{
    if( totalWidth == -1 || totalWidth == width() ) //no change needed
        return;

    QString prettyLength = MetaBundle::prettyTime( m_track->length(), false );
    QString album;
    if( m_track->album() )
        album = m_track->album()->name();

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
    m_topRightText->setPos( rightAlignX, 0.0 );
    m_bottomRightText->setPos( rightAlignX, lineTwoY );
    m_topRightText->setPlainText( s_fm->elidedText( album, Qt::ElideRight, totalWidth - rightAlignX ) );
    m_bottomRightText->setPlainText( s_fm->elidedText( prettyLength, Qt::ElideRight, totalWidth - rightAlignX ) );


    qreal spaceForLeft = totalWidth - ( totalWidth - rightAlignX ) - leftAlignX;
    {
        QString artist;
        if( m_track->artist() )
            artist = m_track->artist()->name();
        m_topLeftText->setPlainText( s_fm->elidedText( artist, Qt::ElideRight, spaceForLeft ) );
        m_topLeftText->setPos( leftAlignX, 0.0 );
    }

    m_bottomLeftText->setPlainText( s_fm->elidedText( QString("%1 - %2").arg( QString::number( m_track->trackNumber() ), m_track->name() )
        , Qt::ElideRight, spaceForLeft ) );
    m_bottomLeftText->setPos( leftAlignX, lineTwoY );
}
