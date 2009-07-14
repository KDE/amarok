/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_PLAYLISTSORTSCHEME_H
#define AMAROK_PLAYLISTSORTSCHEME_H

#include "playlist/PlaylistDefines.h"

#include <QSortFilterProxyModel>
#include <QStack>

namespace Playlist
{

/**
 * A sorting level for multilevel playlist sorting. Instances of this class are aggregated
 * by Playlist::SortScheme to describe a way to sort the playlist.
 * @author Téo Mrnjavac <teo.mrnjavac@gmail.com>
 */
class SortLevel     //data structure for each sorting level, ends up in a qstack or vector or list or map or w/e
{
    public:
        SortLevel( int sortCategory = PlaceHolder, Qt::SortOrder sortOrder = Qt::AscendingOrder );
        int category();
        Qt::SortOrder order();
        void setCategory( int sortCategory );
        void setOrder( Qt::SortOrder sortOrder );
        bool isComparable();
        bool isString();
    private:
        int m_category;     //Column from PlaylistDefines.h
        Qt::SortOrder m_order;
};

/**
 * A sorting scheme for multilevel playlist sorting. This class wraps around a QStack to
 * define a way to sort the playlist and is used by Playlist::SortProxy.
 * @author Téo Mrnjavac <teo.mrnjavac@gmail.com>
 */
class SortScheme
{
    public:
        SortScheme();
        SortLevel & level( int i );
        void addLevel( const SortLevel & level );
        void trimToLevel( int lastLevel );        //deletes all the levels up to level # length
        int length();

    private:
        QStack< SortLevel > *m_scheme;
};

}   //namespace Playlist

#endif  //AMAROK_PLAYLISTSORTSCHEME_H
