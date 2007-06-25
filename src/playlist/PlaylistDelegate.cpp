/***************************************************************************
 * copyright            : (C) 2007 Ian Monroe <ian@monroe.nu>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2        *
 *   as published by the Free Software Foundation.                         *
 ***************************************************************************/

#include "PlaylistDelegate.h"
#include "PlaylistModel.h"
#include "PlaylistView.h"

#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QModelIndex>
#include <QPainter>

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
    QGraphicsTextItem* text = new QGraphicsTextItem();
    text->setFont( QFont() );
    #define GETDATA( X ) index.data( X ).toString()
    QString album = GETDATA( Album );
    QString title = GETDATA( Title );
    QString trackn = GETDATA( TrackNumber );
    QString artist = GETDATA( Artist );
    #undef GETDATA
    text->setHtml( i18n("%1 - <b>%2</b> by <b>%3</b> on <b>%4</b>", trackn, title, artist, album ) );
    scene.addItem( text );
    scene.render( painter, option.rect );
    painter->restore();
}

QSize
Delegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
    return QSize( m_view->width(), 25 );
}

#include "PlaylistDelegate.moc"
