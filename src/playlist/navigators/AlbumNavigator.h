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

            /**
             * Empty notification callback for child classes: new albums have been inserted.
             */
            virtual void notifyAlbumsInserted( const QList<Meta::AlbumPtr> insertedAlbums ) = 0;

            /**
             * Convenience function: the album of 'currentItem()'.
             */
            Meta::AlbumPtr currentAlbum() { return currentItem() ? m_model->trackForId( currentItem() )->album() : Meta::AlbumPtr(); }

            QHash<Meta::AlbumPtr, ItemList> m_itemsPerAlbum;    //! For use by child classes. Maintained automatically.
            QList<Meta::AlbumPtr> m_plannedAlbums;    //! For use by child classes. Cleared automatically.

        private:
            static bool itemLessThan( const quint64 &left, const quint64 &right );
    };
}

#endif
