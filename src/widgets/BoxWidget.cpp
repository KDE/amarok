/*
 * Copyright (C) 2017  Malte Veerman <malte.veerman@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 */

#include "BoxWidget.h"

#include <QChildEvent>
#include <QHBoxLayout>
#include <QVBoxLayout>

BoxWidget::BoxWidget( bool vertical, QWidget *parent ) : QFrame( parent )
{
    if( vertical )
        setLayout( new QVBoxLayout );
    else
        setLayout( new QHBoxLayout );

    layout()->setSpacing( 0 );
    layout()->setContentsMargins( 0, 0, 0, 0 );
}

QBoxLayout* BoxWidget::layout() const
{
    return static_cast<QBoxLayout*>( QWidget::layout() );
}

void BoxWidget::childEvent(QChildEvent *event)
{
    switch ( event->type() )
    {
        case QEvent::ChildAdded:
        {
            if ( event->child()->isWidgetType() )
            {
                QWidget *widget = static_cast<QWidget*>( event->child() );
                layout()->addWidget( widget );
            }
            break;
        }
        case QEvent::ChildRemoved:
        {
            if ( event->child()->isWidgetType() )
            {
                QWidget *widget = static_cast<QWidget*>( event->child() );
                layout()->removeWidget( widget );
            }
            break;
        }
        default:
            break;
    }
    QFrame::childEvent( event );
}
