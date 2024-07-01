/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
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

#ifndef AMAROK_PLAYLISTSORTSCHEME_H
#define AMAROK_PLAYLISTSORTSCHEME_H

#include "playlist/PlaylistDefines.h"

#include <QStack>

namespace Playlist
{

/**
 * A sorting level for multilevel playlist sorting. Instances of this class are aggregated
 * by Playlist::SortScheme to describe a way to sort the playlist.
 * @author Téo Mrnjavac <teo@kde.org>
 */
class SortLevel
{
    public:
        explicit SortLevel( Column sortCategory = PlaceHolder, Qt::SortOrder sortOrder = Qt::AscendingOrder );
        Column category() const;
        Qt::SortOrder order() const;
        void setCategory( Column sortCategory );
        void setOrder( Qt::SortOrder sortOrder );
        bool isComparable() const;
        bool isString() const;
        bool isFloat() const;
        QString prettyName() const;
    private:
        Column m_category;     //Column from PlaylistDefines.h
        Qt::SortOrder m_order;
};

/**
 * A sorting scheme for multilevel playlist sorting. This class wraps around a QStack to
 * define a way to sort the playlist and is used by Playlist::SortProxy.
 * @author Téo Mrnjavac <teo@kde.org>
 * @author Konrad Zemek <konrad.zemek@gmail.com>
 */
class SortScheme : private QStack<SortLevel>
{
    public:
        const SortLevel &level( int i ) const;
        void addLevel( const SortLevel &level );

        /** Deletes all the levels up to level # length */
        void trimToLevel( int lastLevel );
        int length() const;

        typedef QStack<SortLevel>::const_iterator const_iterator;

        const_iterator begin() const;
        const_iterator end() const;
};

}   //namespace Playlist

#endif  //AMAROK_PLAYLISTSORTSCHEME_H
