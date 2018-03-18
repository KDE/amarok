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

#ifndef AMAROK_UPCOMING_EVENTS_MAP_WIDGET_H
#define AMAROK_UPCOMING_EVENTS_MAP_WIDGET_H

#include "LastFmEvent.h"

#include <KGraphicsWebView>

class QGraphicsItem;
class UpcomingEventsListWidget;
class UpcomingEventsMapWidgetPrivate;

class UpcomingEventsMapWidget : public KGraphicsWebView
{
    Q_OBJECT
    Q_PROPERTY( int eventCount READ eventCount )
    Q_PROPERTY( bool isLoaded READ isLoaded )
    Q_PROPERTY( LastFmEvent::List events READ events )

public:
    explicit UpcomingEventsMapWidget( QGraphicsItem *parent = 0 );
    ~UpcomingEventsMapWidget();

    bool isLoaded() const;
    int eventCount() const;
    LastFmEvent::List events() const;

    void addEvents( const LastFmEvent::List &events );

    void clear();

public Q_SLOTS:
    void addEvent( const LastFmEventPtr &event );
    void removeEvent( const LastFmEventPtr &event );
    void addEventsListWidget( UpcomingEventsListWidget *widget );
    void removeEventsListWidget( UpcomingEventsListWidget *widget );
    void centerAt( double latitude, double longitude );
    void centerAt( const LastFmVenuePtr &venue );

private:
    UpcomingEventsMapWidgetPrivate *const d_ptr;
    Q_DECLARE_PRIVATE( UpcomingEventsMapWidget )
    Q_DISABLE_COPY( UpcomingEventsMapWidget )

    Q_PRIVATE_SLOT( d_ptr, void _centerAt(QObject*))
    Q_PRIVATE_SLOT( d_ptr, void _linkClicked(QUrl) )
    Q_PRIVATE_SLOT( d_ptr, void _loadFinished(bool) )
    Q_PRIVATE_SLOT( d_ptr, void _init() )
};

#endif // AMAROK_UPCOMING_EVENTS_MAP_WIDGET_H
