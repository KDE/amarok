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

#include <QFontMetrics>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QModelIndex>
#include <QPainter>
#include <QPixmap>
#include <QRectF>

using namespace PlaylistNS;

Delegate::Delegate( View* parent )
    : QAbstractItemDelegate()
    , m_view( parent )
{ 
    //setParent( parent ); 
}

void
Delegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();
    DEBUG_BLOCK
    debug() << option.state << endl;
    //
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
        albumPixmap =  track->album()->image( 50 );
    }
    QString prettyLength = MetaBundle::prettyTime( track->length(), false );
    QGraphicsPixmapItem* pixmap = new QGraphicsPixmapItem(albumPixmap, 0 );
    pixmap->setPos( 0.0, 2.5 );
    QGraphicsTextItem* leftText = new QGraphicsTextItem();
    QGraphicsTextItem* rightText = new QGraphicsTextItem();
    leftText->setFont( QFont() );
    QString artist;
    if( track->artist() )
        artist = track->artist()->name();
    leftText->setHtml( QString("<b>%1</b><br>%2 - %3").arg( artist, QString::number( track->trackNumber() ), track->name() ) );
    leftText->setPos( 52.0, 0.0 );
    rightText->setFont( QFont() );
    rightText->setHtml( QString("<b>%1</b><br>%2").arg( album, prettyLength ) );
    {
        QFontMetrics* fm = new QFontMetrics( QFont() );
        rightText->setPos( option.rect.width() - qMax( fm->width( album ), fm->width( prettyLength ) ), 0.0 );
        delete fm;
    }
    scene.addItem( pixmap );
    scene.addItem( leftText );
    scene.addItem( rightText );
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
    return QSize( m_view->width(), 52 );
}

#include "PlaylistDelegate.moc"
