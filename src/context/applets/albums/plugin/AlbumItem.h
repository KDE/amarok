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

#ifndef AMAROK_ALBUMITEM_H
#define AMAROK_ALBUMITEM_H

#include "core/meta/Observer.h"

#include <QObject>
#include <QStandardItem>

class AlbumItem : public QObject, public QStandardItem, public Meta::Observer
{
    Q_OBJECT

    public:
        AlbumItem();
        ~AlbumItem() override;

        /**
         * Sets the AlbumPtr for this item to associate with
         *
         * @arg album pointer to associate with
         */
        void setAlbum( const Meta::AlbumPtr &albumPtr );

        /**
         * @return the album pointer associated with this item
         */
        Meta::AlbumPtr album() const { return m_album; }

        /**
         * Sets the size of the album art to display
         */
        void setIconSize( const int iconSize );

        /**
         * @return the size of the album art
         */
        int iconSize() const { return m_iconSize; }

        /**
         * Setter to determine whether the item should show the Artist as well as the
         * album name. Used for 'recent albums' listing.
         */
        void setShowArtist( const bool showArtist );

        // overloaded from Meta::Observer
        using Observer::metadataChanged;
        void metadataChanged( const Meta::AlbumPtr &album ) override;

        int type() const override;

        bool operator<( const QStandardItem &other ) const override;

    private Q_SLOTS:
        /** Updates the item after metadataChanged was called.
            We need this indirection to get executed by the UI thread.
        */
        void update();

    private:
        Meta::AlbumPtr m_album;
        int            m_iconSize;
        bool           m_showArtist;
};

#endif // multiple inclusion guard
