/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *      (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.         *
 ***************************************************************************/

#ifndef MEMORYMATCHER_H
#define MEMORYMATCHER_H

#include "MemoryCollection.h"
#include "meta.h"

/**
A helper class for finding items in a MemoryCollection

	@author 
*/
class MemoryMatcher{
    public:
        MemoryMatcher();
        virtual ~MemoryMatcher();
        virtual Meta::TrackList match( MemoryCollection *memColl) = 0;
        virtual Meta::TrackList match( const Meta::TrackList &tracks ) = 0;

        bool isLast() const;
        void setNext( MemoryMatcher *next );
        MemoryMatcher* next() const;

    private:
        MemoryMatcher *m_next;
};


class TrackMatcher : public MemoryMatcher
{
    public:
        TrackMatcher( Meta::TrackPtr track );
        virtual Meta::TrackList match( MemoryCollection *memColl );
        virtual Meta::TrackList match( const Meta::TrackList &tracks );

    private:
        Meta::TrackPtr m_track;
};


class ArtistMatcher : public MemoryMatcher
{
    public:
        ArtistMatcher( Meta::ArtistPtr artist );
        virtual Meta::TrackList match( MemoryCollection *memColl );
        virtual Meta::TrackList match( const Meta::TrackList &tracks );

        virtual Meta::AlbumList matchAlbums( MemoryCollection *memColl );

    private:
        Meta::ArtistPtr m_artist;
};

class AlbumMatcher : public MemoryMatcher
{
    public:
        AlbumMatcher( Meta::AlbumPtr album );
        virtual Meta::TrackList match( MemoryCollection *memColl );
        virtual Meta::TrackList match( const Meta::TrackList &tracks );

    private:
        Meta::AlbumPtr m_album;
};

class GenreMatcher : public MemoryMatcher
{
    public:
        GenreMatcher( Meta::GenrePtr genre );
        virtual Meta::TrackList match( MemoryCollection *memColl );
        virtual Meta::TrackList match( const Meta::TrackList &tracks );

    private:
        Meta::GenrePtr m_genre;
};

class ComposerMatcher : public MemoryMatcher
{
    public:
        ComposerMatcher( Meta::ComposerPtr composer );
        virtual Meta::TrackList match( MemoryCollection *memColl );
        virtual Meta::TrackList match( const Meta::TrackList &tracks );

    private:
        Meta::ComposerPtr m_composer;
};

class YearMatcher : public MemoryMatcher
{
    public:
        YearMatcher( Meta::YearPtr year );
        virtual Meta::TrackList match( MemoryCollection *memColl );
        virtual Meta::TrackList match( const Meta::TrackList &tracks );

    private:
        Meta::YearPtr m_year;
};


#endif
