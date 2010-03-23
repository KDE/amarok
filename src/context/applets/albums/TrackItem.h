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

#include "core/meta/Meta.h"

#include <QStandardItem>

class TrackItem : public QStandardItem, public Meta::Observer
{
    public:
        TrackItem();
        ~TrackItem() { }

        /**
         * Sets the TrackPtr for this item to associate with
         *
         * @arg track pointer to associate with
         */
        void setTrack( Meta::TrackPtr trackPtr );

        /**
         * @return the track pointer associated with this item
         */
        Meta::TrackPtr track() const { return m_track; }

        /**
         * Applies an italic style if the track is the currently
         * playing track
         */
        void italicise();
	
	/**
	 * Applies a bold style if the track is owned by the currently
         * playing artist
         */
        void bold();

        // overloaded from Meta::Observer
        using Observer::metadataChanged;
        virtual void metadataChanged( Meta::TrackPtr track );

        virtual int type() const { return QStandardItem::UserType + 1; }

    private:
        Meta::TrackPtr m_track;
};

#endif // multiple inclusion guard
