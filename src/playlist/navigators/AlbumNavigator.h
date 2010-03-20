/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2008 Soren Harward <stharward@gmail.com>                               *
 * Copyright (c) 2010 Nanno Langstraat <langstr@gmail.com>                              *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef ALBUMNAVIGATOR_H
#define ALBUMNAVIGATOR_H

#include "NonlinearTrackNavigator.h"

#include "Meta.h"

#include <QList>
#include <QSet>

namespace Playlist
{
    /**
     * Base class that offers some standard services for album-oriented navigators.
     */
    class AlbumNavigator : public NonlinearTrackNavigator
    {
        Q_OBJECT

        public:
            AlbumNavigator() { }

        protected:
            //! Overrides from 'NonlinearTrackNavigator'
            void notifyItemsInserted( const QSet<quint64> &insertedItems );
            void notifyItemsRemoved( const QSet<quint64> &removedItems );

            typedef QString AlbumId;    // Not Meta::AlbumPtr but QString '->name()'. Reason: Meta::AlbumPtr doesn't work for meta types that use private album pointers.

            /**
             * The album of an item. Opaque key for bookkeeping.
             */
            AlbumId albumForItem( quint64 item );

            /**
             * Empty notification callback for child classes: new albums have been inserted.
             */
            virtual void notifyAlbumsInserted( const QList<AlbumId> insertedAlbums ) = 0;

            /**
             * Convenience function: the album of 'currentItem()'.
             */
            AlbumId currentAlbum() { return currentItem() ? albumForItem( currentItem() ) : AlbumId(); }

            QHash<AlbumId, ItemList> m_itemsPerAlbum;    //! For use by child classes. Maintained automatically.
            QList<AlbumId> m_plannedAlbums;    //! For use by child classes. Cleared automatically.

        private:
            static bool itemLessThan( const quint64 &left, const quint64 &right );

            /**
             * Cache the album for each playlist item. The reasons:
             *   - By the time 'notifyItemsRemoved()' is called, the items are usually
             *     already gone from the playlist model.
             *   - Currently we don't get a notification when the item's '->track()->album()'
             *     changes. We need to do all bookkeeping with the same value that we saw
             *     during insertion.
             */
            QHash<quint64, AlbumId> m_albumForItem;
    };
}

#endif
