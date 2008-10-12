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


#ifndef AMAROK_ALBUMITEM_H
#define AMAROK_ALBUMITEM_H

#include <meta/Meta.h>

#include <QSize>
#include <QStandardItem>

class AlbumItem : public QStandardItem, public Meta::Observer
{
    public:
        AlbumItem();
        ~AlbumItem() { }

        /**
         * Sets the AlbumPtr for this item to associate with
         *
         * @arg album pointer to associate with
         */
        void setAlbum( Meta::AlbumPtr albumPtr );

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
        virtual void metadataChanged( Meta::Album *album );

        virtual int type() const { return QStandardItem::UserType; }

    private:
        Meta::AlbumPtr m_album;
        int            m_iconSize;
        bool           m_showArtist;
};

#endif // multiple inclusion guard
