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
#include "PlaylistItem.h"
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

Delegate::Delegate( View* parent )
    : QAbstractItemDelegate()
    , m_view( parent )
    
{ 
    //setParent( parent ); 
}

Delegate::~Delegate()
{ }

void
Delegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();
    //if (option.state & QStyle::State_HasFocus)
    if (option.state & QStyle::State_Selected)
         painter->fillRect(option.rect, option.palette.highlight());
    PlaylistNS::Item* item = index.data( ItemRole ).value< PlaylistNS::Item* >();
    //QRectF sourceShifted = scene.sceneRect().moveTopLeft( QPointF( 0.0, 0.0 ) );
    QRectF targetShifted = option.rect;
    targetShifted.moveTopLeft( QPointF( 0.0, 0.0 ) );
    QGraphicsScene* scene = item->scene( option.rect.width() );
    QRectF croppedSource = scene->sceneRect().intersect( targetShifted );
    scene->render( painter, option.rect, croppedSource );
    painter->restore();
}

QSize
Delegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index ) const
{
    Q_UNUSED( option );
    Q_UNUSED( index );

    return QSize( m_view->width() - m_view->verticalScrollBar()->width(), int( Item::height() ) );
}

#include "PlaylistDelegate.moc"
