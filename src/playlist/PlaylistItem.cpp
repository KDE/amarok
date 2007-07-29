/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "debug.h"
#include "metabundle.h"
#include "PlaylistItem.h"

#include <QFontMetricsF>
#include <QGraphicsScene>
#include <QGraphicsTextItem>

const qreal PlaylistNS::Item::ALBUM_WIDTH = 50.0;
const qreal PlaylistNS::Item::MARGIN = 2.0;
qreal PlaylistNS::Item::s_height = -1.0;
QFontMetricsF* PlaylistNS::Item::s_fm = 0;

PlaylistNS::Item::Item( Meta::TrackPtr track )
    : m_track( track )
    , m_scene( 0 )
{
    if( not s_fm )
    {
        s_fm = new QFontMetricsF( QFont() );
        s_height =  qMax( ALBUM_WIDTH, s_fm->height() * 2 );
    }
}

PlaylistNS::Item::~Item()
{
    DEBUG_BLOCK
    delete m_scene;
}

QGraphicsScene* 
PlaylistNS::Item::scene( int totalWidth )
{
    if( m_scene )
        return m_scene;
    if( not m_track )
        return 0;
    if( totalWidth == -1 && m_scene )
        totalWidth = (int)m_scene->width();
    m_scene = new QGraphicsScene();
    QString album;
    QPixmap albumPixmap;
    if( m_track->album() )
    {
        album = m_track->album()->name();
        albumPixmap =  m_track->album()->image( int( ALBUM_WIDTH ) );
    }
    QString prettyLength = MetaBundle::prettyTime( m_track->length(), false );

    QGraphicsPixmapItem* pixmap = new QGraphicsPixmapItem(albumPixmap, 0 );
    pixmap->setPos( 0.0, 0.0 );

    const qreal lineTwoY = s_height / 2;
    const qreal textWidth = ( ( qreal( totalWidth ) - ALBUM_WIDTH ) / 2.0 );
    const qreal leftAlignX = ALBUM_WIDTH + MARGIN;

    QFont font;
    font.setPointSize( font.pointSize() - 1 );

    QGraphicsTextItem* topLeftText = new QGraphicsTextItem();
    topLeftText->setTextInteractionFlags( Qt::TextEditorInteraction );
    topLeftText->setFont( font );
    QGraphicsTextItem* bottomLeftText = new QGraphicsTextItem();
    bottomLeftText->setTextInteractionFlags( Qt::TextEditorInteraction );
    bottomLeftText->setFont( font );
    QGraphicsTextItem* topRightText = new QGraphicsTextItem();
    topRightText->setFont( font );
    QGraphicsTextItem* bottomRightText = new QGraphicsTextItem();
    bottomRightText->setFont( font );

    qreal rightAlignX;
    {
        qreal middle = textWidth + ALBUM_WIDTH + ( MARGIN * 2.0 );
        qreal rightWidth = totalWidth - qMax( s_fm->width( album )
            , s_fm->width( prettyLength ) );
        rightAlignX = qMax( middle, rightWidth );
    }
    topRightText->setPos( rightAlignX, 0.0 );
    bottomRightText->setPos( rightAlignX, lineTwoY );
    topRightText->setPlainText( s_fm->elidedText( album, Qt::ElideRight, totalWidth - rightAlignX ) );
    bottomRightText->setPlainText( s_fm->elidedText( prettyLength, Qt::ElideRight, totalWidth - rightAlignX ) );


    qreal spaceForLeft = totalWidth - ( totalWidth - rightAlignX ) - leftAlignX;
    {
        QString artist;
        if( m_track->artist() )
            artist = m_track->artist()->name();
        topLeftText->setPlainText( s_fm->elidedText( artist, Qt::ElideRight, spaceForLeft ) );
        topLeftText->setPos( leftAlignX, 0.0 );
    }

    bottomLeftText->setPlainText( s_fm->elidedText( QString("%1 - %2").arg( QString::number( m_track->trackNumber() ), m_track->name() )
        , Qt::ElideRight, spaceForLeft ) );
    bottomLeftText->setPos( leftAlignX, lineTwoY );

    m_scene->addItem( pixmap );
    m_scene->addItem( topLeftText );
    m_scene->addItem( bottomLeftText );
    m_scene->addItem( topRightText );
    m_scene->addItem( bottomRightText );
    return m_scene;
}

