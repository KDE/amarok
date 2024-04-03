/****************************************************************************************
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
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

#ifndef AMAROK_TRACKITEM_H
#define AMAROK_TRACKITEM_H

#include "core/meta/Observer.h"

#include <QStandardItem>
#include <QMutex>

class TrackItem : public QStandardItem, public Meta::Observer
{
    public:
        TrackItem();
        ~TrackItem() override;

        /**
         * Sets the TrackPtr for this item to associate with
         *
         * @arg track pointer to associate with
         */
        void setTrack( const Meta::TrackPtr &trackPtr );

        /**
         * @return the track pointer associated with this item
         */
        Meta::TrackPtr track() const { return m_track; }

        /**
         * Applies an italic style if the track is the currently
         * playing track
         */
        void italicise();
        bool italic() const { return m_italic; }
	
        /**
         * Applies a bold style if the track is owned by the currently
         * playing artist
         */
        void bolden();
        bool bold() const { return m_bold; }

        // overloaded from Meta::Observer
        using Observer::metadataChanged;
        void metadataChanged( const Meta::TrackPtr &track ) override;

        int type() const override;

        bool operator<( const QStandardItem &other ) const override;

    private:
        Meta::TrackPtr m_track;
        bool m_bold;
        bool m_italic;
        QMutex m_mutex;
};

#endif // multiple inclusion guard
