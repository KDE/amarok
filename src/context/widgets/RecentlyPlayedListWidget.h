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

#ifndef RECENTLYPLAYEDLISTWIDGET_H
#define RECENTLYPLAYEDLISTWIDGET_H

#include "core/meta/forward_declarations.h"

#include <KIcon>
#include <Plasma/ScrollWidget>

class QGraphicsLinearLayout;

class RecentlyPlayedListWidget : public Plasma::ScrollWidget
{
    Q_OBJECT

public:
    explicit RecentlyPlayedListWidget( QGraphicsWidget *parent = 0 );
    virtual ~RecentlyPlayedListWidget();

    void clear();

private slots:
    void tracksReturned( Meta::TrackList );
    void trackChanged( Meta::TrackPtr track );
    void setupTracksData();
    void startQuery();
    void updateWidget();

private:
    void addTrack( const Meta::TrackPtr &track );
    void removeItem( QGraphicsLayoutItem *item );

    KIcon m_trackIcon;
    Meta::TrackPtr m_currentTrack;
    QMap<uint, Meta::TrackPtr> m_recentTracks;
    QGraphicsLinearLayout *m_layout;
    Q_DISABLE_COPY( RecentlyPlayedListWidget )
};

#endif // RECENTLYPLAYEDLISTWIDGET_H
