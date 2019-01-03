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

#ifndef AMAROK_UPCOMINGEVENTSCALENDARWIDGET_H
#define AMAROK_UPCOMINGEVENTSCALENDARWIDGET_H

#include "LastFmEvent.h"

#include <QGraphicsProxyWidget>

class UpcomingEventsCalendarWidgetPrivate;

class UpcomingEventsCalendarWidget : public QGraphicsProxyWidget
{
    Q_OBJECT
    Q_PROPERTY( LastFmEvent::List events READ events )
    Q_PROPERTY( QAction* todayAction READ todayAction )

public:
    explicit UpcomingEventsCalendarWidget( QGraphicsItem *parent = nullptr, Qt::WindowFlags wFlags = 0 );
    ~UpcomingEventsCalendarWidget();

    void clear();
    LastFmEvent::List events() const;
    QAction *todayAction();

public Q_SLOTS:
    void addEvent( const LastFmEventPtr &event );
    void addEvents( const LastFmEvent::List &events );

private:
    UpcomingEventsCalendarWidgetPrivate *const d_ptr;
    Q_DECLARE_PRIVATE( UpcomingEventsCalendarWidget )
    Q_DISABLE_COPY( UpcomingEventsCalendarWidget )

    Q_PRIVATE_SLOT( d_ptr, void _paletteChanged(QPalette) )
    Q_PRIVATE_SLOT( d_ptr, void _jumpToToday() )
    Q_PRIVATE_SLOT( d_ptr, void _updateToday() )
};

#endif /* AMAROK_UPCOMINGEVENTSCALENDARWIDGET_H */
