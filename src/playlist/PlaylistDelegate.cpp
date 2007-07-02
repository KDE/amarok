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
    if (option.state & QStyle::State_Selected)
         painter->fillRect(option.rect, option.palette.highlight());
    QGraphicsScene scene;

    QString album = index.data( PlaylistNS::Album ).toString();
    QString title = index.data( Title ).toString();
    QString artist = index.data( Artist ).toString();
    QString trackn = QString::number( index.data( TrackNumber ).toInt() );
    QPixmap cover = index.data( CoverImage ).value<QPixmap>();
    QString prettyLength = MetaBundle::prettyTime( index.data( Length ).toInt(), false );
    QGraphicsPixmapItem* pixmap = new QGraphicsPixmapItem( cover, 0, &scene );
    QGraphicsTextItem* leftText = new QGraphicsTextItem();
    QGraphicsTextItem* rightText = new QGraphicsTextItem();
    leftText->setFont( QFont() );
    leftText->setHtml( QString("<b>%1</b><br>%2 - %3").arg( artist, trackn, title ) );
    leftText->setPos( 52.0, 0.0 );
    rightText->setFont( QFont() );
    rightText->setHtml( QString("<b>%1</b><br>%1").arg( album, prettyLength ) );
    {
        QFontMetrics* fm = new QFontMetrics( QFont() );
        rightText->setPos( option.rect.width() - qMax( fm->width( album ), fm->width( prettyLength ) ), 0.0 );
        delete fm;
    }
    scene.addItem( pixmap );
    scene.addItem( leftText );
    scene.addItem( rightText );
    scene.render( painter, option.rect );
    painter->restore();
}

QSize
Delegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
    return QSize( m_view->width(), 52 );
}

#include "PlaylistDelegate.moc"
