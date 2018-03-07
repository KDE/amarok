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

#ifndef UPCOMINGEVENTSSTACK_H
#define UPCOMINGEVENTSSTACK_H 

#include <QGraphicsWidget>
#include <QIcon>

class UpcomingEventsStackItem;
class UpcomingEventsStackPrivate;

class UpcomingEventsStack : public QGraphicsWidget
{
    Q_OBJECT
    Q_PROPERTY( int count READ count )
    Q_PROPERTY( bool empty READ isEmpty )

public:
    explicit UpcomingEventsStack( QGraphicsItem *parent = 0, Qt::WindowFlags wFlags = 0 );
    ~UpcomingEventsStack();

    int count() const; ///< number of list widgets in the stack widget

    void clear(); ///< clears the stack widget

    bool isEmpty() const; ///< whether the stack is empty

    bool hasItem( const QString &name ) const;

    UpcomingEventsStackItem *item( const QString &name ) const;
    QList<UpcomingEventsStackItem *> items( const QRegExp &pattern ) const;

    UpcomingEventsStackItem *create( const QString &name ); ///< creates a new stack item
    void remove( const QString &name ); ///< remove stack item with name

public Q_SLOTS:
    void maximizeItem( const QString &name ); ///< expand item with name and collapse all else
    void cleanupListWidgets();

Q_SIGNALS:
    void collapseStateChanged();

private:
    UpcomingEventsStackPrivate *const d_ptr;
    Q_DECLARE_PRIVATE( UpcomingEventsStack )
    Q_DISABLE_COPY( UpcomingEventsStack )

    Q_PRIVATE_SLOT( d_ptr, void _itemDestroyed() )
};

#endif /* UPCOMINGEVENTSSTACK_H */
