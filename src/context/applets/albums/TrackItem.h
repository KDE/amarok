/*******************************************************************************
* copyright              : (C) 2008 Seb Ruiz <ruiz@kde.org>                    *
*                                                                              *
********************************************************************************/

/*******************************************************************************
*                                                                              *
*   This program is free software; you can redistribute it and/or modify       *
*   it under the terms of the GNU General Public License as published by       *
*   the Free Software Foundation; either version 2 of the License, or          *
*   (at your option) any later version.                                        *
*                                                                              *
********************************************************************************/


#ifndef AMAROK_TRACKITEM_H
#define AMAROK_TRACKITEM_H

#include <meta/Meta.h>

#include <QSize>
#include <QStandardItem>

class TrackItem : public QStandardItem, public Meta::Observer
{
    public:
        TrackItem() : QStandardItem() { }

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

        // overloaded from Meta::Observer
        using Observer::metadataChanged;
        virtual void metadataChanged( Meta::Track *track );

    private:
        Meta::TrackPtr m_track;
};

#endif // multiple inclusion guard
