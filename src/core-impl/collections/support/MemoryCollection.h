/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef MEMORYCOLLECTION_H
#define MEMORYCOLLECTION_H

#include "core/meta/Meta.h"
#include "core/meta/support/MetaKeys.h"

#include <QReadWriteLock>

//QMap is slower than QHash but the items are ordered by key
typedef QMap<QString, Meta::TrackPtr> TrackMap;
typedef QMap<QString, Meta::ArtistPtr> ArtistMap;
class AlbumMap : public QMap<Meta::AlbumKey, Meta::AlbumPtr>
{
    public:
        /**
         * Return true if this map contains album with same name and artist as @param album
         *
         * This is a convenience overload of contains( QString, QString ) that has same
         * semantics, i.e. compares albums by name and artist name.
         */
        bool contains( const Meta::AlbumPtr &album ) const {
            return QMap<Meta::AlbumKey, Meta::AlbumPtr>::contains( Meta::AlbumKey( album ) ); }

        /**
         * @return @c true if this map contains album named @param name with album artist
         * @param artistName Amarok convention is to use empty artistName for compilations.
         */
        bool contains( const QString &name, const QString &artistName ) const {
            return QMap<Meta::AlbumKey, Meta::AlbumPtr>::contains( Meta::AlbumKey( name, artistName ) ); }

        /**
         * Inserts new album @param album into this map, using its name and album artist.
         * If album has no album artist, empty QString is used as artist key. You must not
         * change name and album artist of album after adding it to this map!
         */
        void insert( const Meta::AlbumPtr &album ) {
            QMap<Meta::AlbumKey, Meta::AlbumPtr>::insert( Meta::AlbumKey( album ) , album ); }

        /**
         * Remove album from this map that has same name and artist as @param album
         */
        int remove( const Meta::AlbumPtr &album ) {
            return QMap<Meta::AlbumKey, Meta::AlbumPtr>::remove( Meta::AlbumKey( album ) ); }

        /**
         * Return pointer to album from this map that has same name and artist as @param album
         *
         * This is a convenience overload of value( QString, QString ) that has same
         * semantics, i.e. compares albums by name and artist name.
         */
        const Meta::AlbumPtr value( const Meta::AlbumPtr &album ) const {
            return QMap<Meta::AlbumKey, Meta::AlbumPtr>::value( Meta::AlbumKey( album ) ); }

        /**
         * Return pointer to album from this map that has name @param name and
         * album artist @param artistName
         */
        const Meta::AlbumPtr value( const QString &name, const QString &artistName ) const {
            return QMap<Meta::AlbumKey, Meta::AlbumPtr>::value( Meta::AlbumKey( name, artistName ) ); }
};
typedef QMap<QString, Meta::GenrePtr> GenreMap;
typedef QMap<QString, Meta::ComposerPtr> ComposerMap;
typedef QMap<int, Meta::YearPtr> YearMap;
typedef QMap<QString, Meta::LabelPtr> LabelMap;
typedef QHash<Meta::LabelPtr, Meta::TrackList> LabelToTrackMap;

namespace Collections {

class MemoryCollection
{
    public:
        void acquireReadLock() { m_readWriteLock.lockForRead(); }
        void releaseLock() { m_readWriteLock.unlock(); }
        void acquireWriteLock() { m_readWriteLock.lockForWrite(); }

        const TrackMap &trackMap() const { return m_trackMap; }
        const ArtistMap &artistMap() const { return m_artistMap; }
        const AlbumMap &albumMap() const { return m_albumMap; }
        const GenreMap &genreMap() const { return m_genreMap; }
        const ComposerMap &composerMap() const { return m_composerMap; }
        const YearMap &yearMap() const { return m_yearMap; }
        const LabelMap &labelMap() const { return m_labelMap; }
        const LabelToTrackMap &labelToTrackMap() const { return m_labelToTrackMap; }

        void setTrackMap( const TrackMap &map ) { m_trackMap = map; }
        void addTrack( Meta::TrackPtr trackPtr ) { m_trackMap.insert( trackPtr->uidUrl(), trackPtr ); }
        void setArtistMap( const ArtistMap &map ) { m_artistMap = map; }
        void addArtist( Meta::ArtistPtr artistPtr) { m_artistMap.insert( artistPtr->name(), artistPtr ); }
        void setAlbumMap( const AlbumMap &map ) { m_albumMap = map; }
        void addAlbum ( Meta::AlbumPtr albumPtr ) { m_albumMap.insert( albumPtr ); }
        void setGenreMap( GenreMap map ) { m_genreMap = map; }
        void addGenre( Meta::GenrePtr genrePtr) { m_genreMap.insert( genrePtr->name(), genrePtr ); }
        void setComposerMap( const ComposerMap &map ) { m_composerMap = map; }
        void addComposer( Meta::ComposerPtr composerPtr ) { m_composerMap.insert( composerPtr->name(), composerPtr ); }
        void setYearMap( const YearMap &map ) { m_yearMap = map; }
        void addYear( Meta::YearPtr yearPtr ) { m_yearMap.insert( yearPtr->year(), yearPtr ); }
        void clearLabels()
        {
            m_labelMap = LabelMap();
            m_labelToTrackMap = LabelToTrackMap();
        }

        void addLabelToTrack( const Meta::LabelPtr &labelPtr, const Meta::TrackPtr &track )
        {
            m_labelMap.insert( labelPtr->name(), labelPtr );
            Meta::TrackList tracks;
            if( m_labelToTrackMap.contains( labelPtr ) )
            {
                tracks = m_labelToTrackMap.value( labelPtr );
            }
            tracks << track;
            m_labelToTrackMap.insert( labelPtr, tracks );
        }

        /**
         * Return a pointer to MemoryCollection's internal lock. Useful to use
         * QReadWriteLocker instead of acquireRead/WriteLock() and releaseLock()
         */
        QReadWriteLock *mapLock() const { return &m_readWriteLock; }

    protected:
        mutable QReadWriteLock m_readWriteLock;
        TrackMap m_trackMap;
        ArtistMap m_artistMap;
        AlbumMap m_albumMap;
        GenreMap m_genreMap;
        ComposerMap m_composerMap;
        YearMap m_yearMap;
        LabelMap m_labelMap;
        LabelToTrackMap m_labelToTrackMap;

};

} //namespace Collections

#endif
