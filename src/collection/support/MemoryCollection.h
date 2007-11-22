/*
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef MEMORYCOLLECTION_H
#define MEMORYCOLLECTION_H

#include "meta/Meta.h"

#include <QReadWriteLock>

//QMap is slower than QHash but the items are ordered by key
typedef QMap<QString, Meta::TrackPtr> TrackMap;
typedef QMap<QString, Meta::ArtistPtr> ArtistMap;
typedef QMap<QString, Meta::AlbumPtr> AlbumMap;
typedef QMap<QString, Meta::GenrePtr> GenreMap;
typedef QMap<QString, Meta::ComposerPtr> ComposerMap;
typedef QMap<QString, Meta::YearPtr> YearMap;


class MemoryCollection
{
    public:

        void acquireReadLock() { m_readWriteLock.lockForRead(); }
        void releaseLock() { m_readWriteLock.unlock(); }
        void acquireWriteLock() { m_readWriteLock.lockForWrite(); }

        TrackMap trackMap() { return m_trackMap; }
        ArtistMap artistMap() { return m_artistMap; }
        AlbumMap albumMap() { return m_albumMap; }
        GenreMap genreMap() { return m_genreMap; }
        ComposerMap composerMap() { return m_composerMap; }
        YearMap yearMap() { return m_yearMap; }

        void setTrackMap( TrackMap map ) { m_trackMap = map; }
        void addTrack( QString key, Meta::TrackPtr trackPtr ) { m_trackMap.insert( key, trackPtr ); }
        void setArtistMap( ArtistMap map ) { m_artistMap = map; }
        void addArtist( QString key, Meta::ArtistPtr artistPtr) { m_artistMap.insert( key, artistPtr ); }
        void setAlbumMap( AlbumMap map ) { m_albumMap = map; }
        void addAlbum ( QString key, Meta::AlbumPtr albumPtr ) { m_albumMap.insert( key, albumPtr ); }
        void setGenreMap( GenreMap map ) { m_genreMap = map; }
        void addGenre( QString key, Meta::GenrePtr genrePtr) { m_genreMap.insert( key, genrePtr ); }
        void setComposerMap( ComposerMap map ) { m_composerMap = map; }
        void addComposer( QString key, Meta::ComposerPtr composerPtr ) { m_composerMap.insert( key, composerPtr ); }
        void setYearMap( YearMap map ) { m_yearMap = map; }
        void addYear( QString key, Meta::YearPtr yearPtr ) { m_yearMap.insert( key, yearPtr ); }

    protected:
        QReadWriteLock m_readWriteLock;
        TrackMap m_trackMap;
        ArtistMap m_artistMap;
        AlbumMap m_albumMap;
        GenreMap m_genreMap;
        ComposerMap m_composerMap;
        YearMap m_yearMap;

};

#endif
