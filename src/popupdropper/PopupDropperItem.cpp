/***************************************************************************
 *   Copyright (c) 2008  Jeff Mitchell <mitchell@kde.org>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include <QtGui>
#include <QtDebug>
#include <QtSvg/QSvgRenderer>
#include <QtSvg/QGraphicsSvgItem>
#include <QFont>

#include "PopupDropperItem.h"
#include "PopupDropperItem_p.h"
#include "PopupDropperAction.h"

///////////////////////////////////////////////////////////

PopupDropperItem::PopupDropperItem( QGraphicsItem *parent )
    : QGraphicsSvgItem( parent )
    , d( new PopupDropperItemPrivate( this ) )
{
    connect( &d->hoverTimer, SIGNAL( timeout() ), this, SLOT( hoverTimeout() ) );
}

PopupDropperItem::PopupDropperItem( const QString &file, QGraphicsItem *parent )
    : QGraphicsSvgItem( file, parent )
    , d( new PopupDropperItemPrivate( this ) )
{
    connect( &d->hoverTimer, SIGNAL( timeout() ), this, SLOT( hoverTimeout() ) );
}

PopupDropperItem::~PopupDropperItem()
{
    //qDebug() << "Deleting popupdropperitem with text = " << d->text;
    delete d;
}

void PopupDropperItem::show()
{
    QGraphicsSvgItem::show();
}

QAction* PopupDropperItem::action() const
{
    return d->action;
}

//warning: setting a PopupDropperAction will override any currently defined shared renderer
//and element id, if they exist in the action!
void PopupDropperItem::setAction( QAction *action )
{
    //note that this also sets the text
    d->action = action;
    d->text = action->iconText();
    PopupDropperAction* pudaction = dynamic_cast<PopupDropperAction*>(action);
    if( pudaction )
    {
        if( pudaction->renderer() && pudaction->renderer()->isValid() )
            QGraphicsSvgItem::setSharedRenderer( pudaction->renderer() );
        if( !pudaction->elementId().isEmpty() )
            QGraphicsSvgItem::setElementId( pudaction->elementId() );
    }
}

QString PopupDropperItem::text() const
{
    return d->text;
}

void PopupDropperItem::setText( const QString &text )
{
    d->text = text;
}

QFont PopupDropperItem::font() const
{
    return d->font;
}

void PopupDropperItem::setFont( const QFont &font )
{
    d->font = font;
    if( d->textItem )
        d->textItem->setFont( font );
}

QGraphicsTextItem* PopupDropperItem::textItem() const
{
    return d->textItem;
}

void PopupDropperItem::setTextItem( QGraphicsTextItem *textItem )
{
    d->textItem = textItem;
}

void PopupDropperItem::reposTextItem()
{
    if( d->textItem )
    {
        d->textItem->setPos( 20 + boundingRect().width(), ( boundingRect().height() / 2 ) - ( d->textItem->boundingRect().height() / 2 ) );
        d->textItem->setFont( d->font );
    }
}

QSvgRenderer* PopupDropperItem::sharedRenderer() const
{
    return QGraphicsSvgItem::renderer();
}

void PopupDropperItem::setSharedRenderer( QSvgRenderer *renderer )
{
    if( renderer )
        QGraphicsSvgItem::setSharedRenderer( renderer );
}

QString PopupDropperItem::elementId() const
{
    return QGraphicsSvgItem::elementId();
}

void PopupDropperItem::setElementId( const QString &id )
{
    //qDebug() << "Element ID being set: " << id;
    QGraphicsSvgItem::setElementId( id );
}

int PopupDropperItem::hoverMsecs() const
{
    return d->hoverMsecs;
}

void PopupDropperItem::setHoverMsecs( const int msecs )
{
    d->hoverMsecs = msecs;
}

void PopupDropperItem::startHoverTimer()
{
    d->hoverTimer.start( d->hoverMsecs );
}

void PopupDropperItem::stopHoverTimer()
{
    d->hoverTimer.stop();
}

bool PopupDropperItem::submenuTrigger() const
{
    return d->submenuTrigger;
}

void PopupDropperItem::setSubmenuTrigger( bool trigger )
{
    d->submenuTrigger = trigger;
}

bool PopupDropperItem::operator<( const PopupDropperItem &other ) const
{
    return d->text < other.text();
}

void PopupDropperItem::dropped( QDropEvent *event ) //virtual SLOT
{
    Q_UNUSED( event );
    d->hoverTimer.stop();
    qDebug() << "PopupDropperItem drop detected";
    if( d->action )
    {
        qDebug() << "Triggering action";
        d->action->activate( QAction::Trigger );
    }
    qDebug() << "emitting dropEvent";
    emit dropEvent( event );
}

void PopupDropperItem::hoverTimeout() //SLOT
{
    qDebug() << "PopupDropperItem timeout";
    if( d->action )
        d->action->activate( QAction::Hover );
}

#include "PopupDropperItem.moc"

