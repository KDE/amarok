/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "debug.h"
#include "metabundle.h"
#include "PlaylistDelegate.h"
#include "PlaylistModel.h"
#include "PlaylistView.h"

#include <QFontMetricsF>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QModelIndex>
#include <QPainter>
#include <QPixmap>
#include <QRectF>
#include <QScrollBar>

using namespace PlaylistNS;

const qreal Delegate::ALBUM_WIDTH = 50.0;
const qreal Delegate::MARGIN = 2.0;

Delegate::Delegate( View* parent )
    : QAbstractItemDelegate()
    , m_view( parent )
    , m_fm( new QFontMetricsF( QFont() ) )
{ 
    //setParent( parent ); 
    m_height = qMax( ALBUM_WIDTH, m_fm->height() * 2 );
}

Delegate::~Delegate()
{
    delete m_fm;
}

void
Delegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();
    //if (option.state & QStyle::State_HasFocus)
    if (option.state & QStyle::State_Selected)
         painter->fillRect(option.rect, option.palette.highlight());
    QGraphicsScene scene;
    Meta::TrackPtr track = index.data( TrackRole ).value< Meta::TrackPtr >();
    if( !track )
        return;
    QString album;
    QPixmap albumPixmap;
    if( track->album() )
    {
        album = track->album()->name();
        albumPixmap =  track->album()->image( int( ALBUM_WIDTH ) );
    }
    QString prettyLength = MetaBundle::prettyTime( track->length(), false );

    QGraphicsPixmapItem* pixmap = new QGraphicsPixmapItem(albumPixmap, 0 );
    pixmap->setPos( 0.0, 0.0 );

//    QFont boldFont;
//    boldFont.setBold( true );
    const qreal lineTwoY = m_height / 2;
    const qreal textWidth = ( ( qreal( option.rect.width() ) - ALBUM_WIDTH ) / 2.0 );
    const qreal totalWidth = qreal( option.rect.width() );
    const qreal leftAlignX = ALBUM_WIDTH + MARGIN;

    QGraphicsTextItem* topLeftText = new QGraphicsTextItem();
    topLeftText->setTextInteractionFlags( Qt::TextEditorInteraction );
//    topLeftText->setFont( boldFont );
    QGraphicsTextItem* bottomLeftText = new QGraphicsTextItem();
    bottomLeftText->setTextInteractionFlags( Qt::TextEditorInteraction );
    QGraphicsTextItem* topRightText = new QGraphicsTextItem();
//    topRightText->setFont( boldFont );
    QGraphicsTextItem* bottomRightText = new QGraphicsTextItem();
//    bottomRightText->setFont( boldFont );

    qreal rightAlignX;
    {
        qreal middle = textWidth + ALBUM_WIDTH + ( MARGIN * 2.0 );
        qreal rightWidth = totalWidth - qMax( m_fm->width( album )
            , m_fm->width( prettyLength ) );
        rightAlignX = qMax( middle, rightWidth );
    }
    topRightText->setPos( rightAlignX, 0.0 );
    bottomRightText->setPos( rightAlignX, lineTwoY );
    topRightText->setPlainText( m_fm->elidedText( album, Qt::ElideRight, totalWidth - rightAlignX ) );
    bottomRightText->setPlainText( m_fm->elidedText( prettyLength, Qt::ElideRight, totalWidth - rightAlignX ) );


    qreal spaceForLeft = totalWidth - ( totalWidth - rightAlignX ) - leftAlignX;
    {
        QString artist;
        if( track->artist() )
            artist = track->artist()->name();
        topLeftText->setPlainText( m_fm->elidedText( artist, Qt::ElideRight, spaceForLeft ) );
        topLeftText->setPos( leftAlignX, 0.0 );
    }
    bottomLeftText->setPlainText( m_fm->elidedText( QString("%1 - %2").arg( QString::number( track->trackNumber() ), track->name() )
        , Qt::ElideRight, spaceForLeft ) );
    bottomLeftText->setPos( leftAlignX, lineTwoY );

    scene.addItem( pixmap );
    scene.addItem( topLeftText );
    scene.addItem( bottomLeftText );
    scene.addItem( topRightText );
    scene.addItem( bottomRightText );
    //QRectF sourceShifted = scene.sceneRect().moveTopLeft( QPointF( 0.0, 0.0 ) );
    QRectF targetShifted = option.rect;
    targetShifted.moveTopLeft( QPointF( 0.0, 0.0 ) );
    QRectF croppedSource = scene.sceneRect().intersect( targetShifted );
    scene.render( painter, option.rect, croppedSource );
    painter->restore();
}

QSize
Delegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
    return QSize( m_view->width() - m_view->verticalScrollBar()->width(), int( m_height ) );
}

#include "PlaylistDelegate.moc"
