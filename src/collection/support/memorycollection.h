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

#include "meta.h"

#include <QReadWriteLock>

using namespace Meta;

//QMap is slower than QHash but the items are ordered by key
typedef QMap<QString, TrackPtr> TrackMap;
typedef QMap<QString, ArtistPtr> ArtistMap;
typedef QMap<QString, AlbumPtr> AlbumMap;
typedef QMap<QString, GenrePtr> GenreMap;
typedef QMap<QString, ComposerPtr> ComposerMap;
typedef QMap<QString, YearPtr> YearMap;


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
        void setArtistMap( ArtistMap map ) { m_artistMap = map; }
        void setAlbumMap( AlbumMap map ) { m_albumMap = map; }
        void setGenreMap( GenreMap map ) { m_genreMap = map; }
        void setComposerMap( ComposerMap map ) { m_composerMap = map; }
        void setYearMap( YearMap map ) { m_yearMap = map; }

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
