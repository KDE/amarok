/****************************************************************************************
 * Copyright (c) 2010 Rick W. Chen <stuffcorpse@archlinux.us>                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "UpcomingEventsStack"

#include "UpcomingEventsStack.h"
#include "UpcomingEventsStackItem.h"
#include "core/support/Debug.h"

#include <QGraphicsLinearLayout>
#include <QSet>
#include <QWeakPointer>

class UpcomingEventsStackPrivate
{
private:
    UpcomingEventsStack *const q_ptr;
    Q_DECLARE_PUBLIC( UpcomingEventsStack )

public:
    UpcomingEventsStackPrivate( UpcomingEventsStack *parent );
    ~UpcomingEventsStackPrivate();

    QGraphicsLinearLayout *layout;
    QHash< QString, QWeakPointer<UpcomingEventsStackItem> > items;

    void _itemDestroyed()
    {
        QHashIterator< QString, QWeakPointer<UpcomingEventsStackItem> > i(items);
        while( i.hasNext() )
        {
            i.next();
            if( i.value().isNull() )
                items.remove( i.key() );
        }
    }
};

UpcomingEventsStackPrivate::UpcomingEventsStackPrivate( UpcomingEventsStack *parent )
    : q_ptr( parent )
    , layout( 0 )
{
}

UpcomingEventsStackPrivate::~UpcomingEventsStackPrivate()
{
}

UpcomingEventsStack::UpcomingEventsStack( QGraphicsItem *parent, Qt::WindowFlags wFlags )
    : QGraphicsWidget( parent, wFlags )
    , d_ptr( new UpcomingEventsStackPrivate( this ) )
{
    Q_D( UpcomingEventsStack );
    d->layout = new QGraphicsLinearLayout( Qt::Vertical, this );
    d->layout->setContentsMargins( 0, 0, 0, 0 );
    setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
}

UpcomingEventsStack::~UpcomingEventsStack()
{
    delete d_ptr;
}

int
UpcomingEventsStack::count() const
{
    Q_D( const UpcomingEventsStack );
    return d->layout->count();
}

void
UpcomingEventsStack::clear()
{
    prepareGeometryChange();
    Q_D( UpcomingEventsStack );
    int itemCount = d->layout->count();
    while( --itemCount >= 0 )
    {
        QGraphicsLayoutItem *child = d->layout->itemAt( 0 );
        d->layout->removeItem( child );
    }
    foreach( QWeakPointer<UpcomingEventsStackItem> item, d->items )
        item.data()->deleteLater();
    d->items.clear();
}

bool
UpcomingEventsStack::isEmpty() const
{
    Q_D( const UpcomingEventsStack );
    return d->items.isEmpty();
}

bool
UpcomingEventsStack::hasItem( const QString &name ) const
{
    Q_D( const UpcomingEventsStack );
    if( d->items.value( name ) )
        return true;
    else
        return false;
}

UpcomingEventsStackItem *
UpcomingEventsStack::item( const QString &name ) const
{
    Q_D( const UpcomingEventsStack );
    Q_ASSERT( d->items.contains( name ) );
    return d->items.value( name ).data();
}

QList<UpcomingEventsStackItem *>
UpcomingEventsStack::items( const QRegExp &pattern ) const
{
    Q_D( const UpcomingEventsStack );
    QList<UpcomingEventsStackItem *> matched;
    QHashIterator< QString, QWeakPointer<UpcomingEventsStackItem> > i( d->items );
    while( i.hasNext() )
    {
        i.next();
        if( i.key().contains( pattern ) )
            matched << i.value().data();
    }
    return matched;
}

UpcomingEventsStackItem *
UpcomingEventsStack::create( const QString &name )
{
    if( hasItem( name ) )
        return 0;

    Q_D( UpcomingEventsStack );
    QWeakPointer<UpcomingEventsStackItem> item = new UpcomingEventsStackItem( name, this );
    d->layout->addItem( item.data() );
    d->items.insert( name, item );
    connect( item.data(), SIGNAL(destroyed()), SLOT(_itemDestroyed()) );
    connect( item.data(), SIGNAL(collapseChanged(bool)), SIGNAL(collapseStateChanged()) );
    return item.data();
}

void
UpcomingEventsStack::remove( const QString &name )
{
    Q_D( UpcomingEventsStack );
    d->items.take( name ).data()->deleteLater();
}

void
UpcomingEventsStack::maximizeItem( const QString &name )
{
    if( hasItem( name ) )
    {
        Q_D( UpcomingEventsStack );
        d->items.value( name ).data()->setCollapsed( false );
        QHashIterator< QString, QWeakPointer<UpcomingEventsStackItem> > i( d->items );
        while( i.hasNext() )
        {
            i.next();
            if( i.value().data()->name() != name )
                i.value().data()->setCollapsed( true );
        }
    }
}

void
UpcomingEventsStack::cleanupListWidgets()
{
    Q_D( UpcomingEventsStack );
    QHashIterator< QString, QWeakPointer<UpcomingEventsStackItem> > i( d->items );
    while( i.hasNext() )
    {
        i.next();
        if( i.value().isNull() )
            d->items.remove( i.key() );
    }
}

