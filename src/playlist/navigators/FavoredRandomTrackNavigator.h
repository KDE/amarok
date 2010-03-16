/****************************************************************************************
 * Copyright (c) 2009 Edward Toroshchin <edward.hades@gmail.com>                        *
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

#ifndef FAVOREDRANDOMTRACKNAVIGATOR_H
#define FAVOREDRANDOMTRACKNAVIGATOR_H

#include "NonlinearTrackNavigator.h"

namespace Playlist
{

    class FavoredRandomTrackNavigator : public NonlinearTrackNavigator
    {
        public:
            FavoredRandomTrackNavigator();

        private:
            //! Override from 'NonlinearTrackNavigator'
            void planOne();
            void notifyItemsInserted( const QSet<quint64> &insertedItems ) { Q_UNUSED( insertedItems ); }
            void notifyItemsRemoved( const QSet<quint64> &removedItems ) { Q_UNUSED( removedItems ); }
    };

}

#endif
