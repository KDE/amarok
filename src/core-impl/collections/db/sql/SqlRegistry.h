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

#ifndef SQLREGISTRY_H
#define SQLREGISTRY_H

#include "SqlMeta.h"
#include "amarok_sqlcollection_export.h"

#include <QHash>
#include <QMutex>
#include <QObject>
#include <QPair>
#include <QTimer>
#include <QList>


class TestSqlAlbum;
class TestSqlArtist;
class TestSqlTrack;
class TestSqlCollectionLocation;

namespace Collections {
    class SqlCollection;
    class SqlQueryMakerInternal;
}
typedef QPair<int, QString> TrackPath;

/** The SqlRegistry class buffers Meta objects from an Sql database.
    This class can be considered a memory cache for the Sql database.
    All requests for Meta objects like SqlTrack should go through here.

    Some notes regarding performance:
    Scanning of nearly 10000 tracks on my lokal disk takes over 2 minutes.
    The second time it only a little over 4 seconds. However I would not see the
    second scan as a valid usecase.
    Putting 10000 tracks from memory directly into the database with
    single inserts takes around 12 seconds.
    This time however increases dramatically with the amount of tracks.
    50000 tracks take around 15 minutes.
    The reason is the many indices that need to be updated. The tracks
    table e.g. has around 15 indices.

    To increase the performance we are currently using two tricks.
    1. Prevent queries.
      The SqlScanResultProcessor will query all tracks in the database.
      The SqlRegistry is caching them as usually but while the scanner
      is running it will not clean the cache.
      The SqlScanResultProcessor will also cache all urls entries.

    2. Combine inserts and updates.
      All dirty tracks will be written in one big insert or with delayed
      updates.
      The ScanResultProcessor will block database access for five
      seconds at a time to collect dirty tracks for such a batch update.
      Have a look at the AbstractTrackTableCommitter to see how this
      is done.

    Note: updating tracks is not optimized for single changes.
    A single update is only done very seldom and does currently not
    need to be optimized.
*/
class AMAROK_SQLCOLLECTION_EXPORT SqlRegistry : public QObject
{
    Q_OBJECT

    public:
        explicit SqlRegistry(Collections::SqlCollection *collection);
        ~SqlRegistry() override;

        /** Searches a directory entry in the scanned directories
            This function searches an existing directory entry.
            @param path the directory path
            @param mtime if mtime is != 0 then the mtime of the entry is updated
            @returns the directory id
         */
        int getDirectory( const QString &path, quint64 mtime = 0 );

        Meta::TrackPtr getTrack( int urlId );
        Meta::TrackPtr getTrack( const QString &path );

        /** Returns the track located at the given url or a new one if not existing.
            This is kind of dangerous because it can generate a new track in
            the database without a file on the filesystem, so don't call it unless
            you really want tracks to be generate.
            The new track must be committed by writing
            some other meta information.
            Use SqlCollection::trackForUrl instead.
         */
        Meta::TrackPtr getTrack( int deviceId, const QString &rpath, int directoryId, const QString &uidUrl );

        /** Returns a track from a specific uid.
            Returns a complete track or 0.
        */
        Meta::TrackPtr getTrackFromUid( const QString &uid );

        Meta::ArtistPtr getArtist( const QString &name );
        Meta::ArtistPtr getArtist( int id );

        Meta::GenrePtr getGenre( const QString &name );
        Meta::GenrePtr getGenre( int id );

        Meta::ComposerPtr getComposer( const QString &name );
        Meta::ComposerPtr getComposer( int id );

        Meta::YearPtr getYear( int year, int yearId = -1 );

        Meta::AlbumPtr getAlbum( const QString &album, const QString &artist );
        Meta::AlbumPtr getAlbum( int id );

        Meta::LabelPtr getLabel( const QString &label );
        Meta::LabelPtr getLabel( int id );

        /** Call this function to collect changes for the sql database.
            This function can be called in preparation of larger updates.
         */
        void blockDatabaseUpdate();

        /** Unblocks one blockDatabaseUpdate call. */
        void unblockDatabaseUpdate();

    private Q_SLOTS:
        /** empytCache clears up the different hash tables by unrefing all pointers that are no longer ref'd by anyone else.
            SqlRegistry is calling this function periodically.
            This is no free ticket for modifying the database directly as
            parties holding Meta pointers will still have the old status.
         */
        void emptyCache();

    private:
        typedef QPair<QString, QString> AlbumKey;

        // only SqlTrack can change this
        /** Updates the uid of an already cached track.
            @return true if the update was successful.
        */
        bool updateCachedUrl( const QString &oldPath, const QString &newPath );

        /** Updates the uid of an already cached track.
            @return true if the update was successful.
        */
        bool updateCachedUid( const QString &oldUid, const QString &newUid );

        /**
         * Removes the track and associated entries (url, statistics, lyrics, labels)
         * from the database and the cache (but not from the file system). This function
         * is normally called by SqlTrack. Do not call directly unless you know what you
         * do.
         */
        void removeTrack( int urlId, const QString &uid );

        // --- functions needed to commit a track

        /** Returns a string with all the values needed to be committed to the urls table */
        QString getTrackUrlsValues( Meta::SqlTrack *track );

        /** Returns a string with all the values needed to be committed to the tracks table */
        QString getTrackTracksValues( Meta::SqlTrack *track );

        /** Returns a string with all the values needed to be committed to the statistics table */
        QString getTrackStatisticsValues( Meta::SqlTrack *track );

        void commitDirtyTracks();



        friend class Meta::SqlTrack;

        // only the query maker creates Metas like this
        Meta::TrackPtr getTrack( int id, const QStringList &rowData );
        Meta::ArtistPtr getArtist( int id, const QString &name );
        Meta::GenrePtr getGenre( int id, const QString &name );
        Meta::ComposerPtr getComposer( int id, const QString &name );
        Meta::AlbumPtr getAlbum( int id, const QString &album, int artistId );
        Meta::LabelPtr getLabel( int id, const QString &label );

        friend class Collections::SqlQueryMakerInternal;

        // we don't care about the ordering so use the faster QHash
        QHash<TrackPath, Meta::TrackPtr > m_trackMap;
        QHash<QString, Meta::TrackPtr > m_uidMap;
        QHash<QString, Meta::ArtistPtr > m_artistMap;
        QHash<int, Meta::ArtistPtr > m_artistIdMap;
        QHash<QString, Meta::ComposerPtr > m_composerMap;
        QHash<QString, Meta::GenrePtr > m_genreMap;
        QHash<int, Meta::YearPtr > m_yearMap;
        QHash<AlbumKey, Meta::AlbumPtr > m_albumMap;
        QHash<int, Meta::AlbumPtr > m_albumIdMap;
        QHash<QString, Meta::LabelPtr > m_labelMap;

        QMutex m_trackMutex; // guards access to m_trackMap, m_uidMap
        QMutex m_artistMutex; // guards access to m_artistMap, m_artistIdMap
        QMutex m_composerMutex; // guards access to m_composerMap
        QMutex m_genreMutex; // guards access to m_genreMap
        QMutex m_yearMutex; // guards access to m_yearMap
        QMutex m_albumMutex; // guards access to m_albumMap, m_albumIdMap
        QMutex m_labelMutex; // guards access to m_labelMap

        /** The timer is used for cleaning up the different caches. */
        QTimer *m_timer;

        Collections::SqlCollection *m_collection;

        QMutex m_blockMutex; // protects the count and all the dirty sets.
        int m_blockDatabaseUpdateCount;

        /** A set of all tracks that need to be written to the database */
        QSet< Meta::SqlTrackPtr > m_dirtyTracks;

        /** A set of all tracks that are dirty.
            Dirty years do not need to be written back as they are
            invariant. However we need to notice the observers and
            invalidate the cache. */
        QSet< Meta::SqlYearPtr > m_dirtyYears;
        QSet< Meta::SqlGenrePtr > m_dirtyGenres;
        QSet< Meta::SqlAlbumPtr > m_dirtyAlbums;
        QSet< Meta::SqlArtistPtr > m_dirtyArtists;
        QSet< Meta::SqlComposerPtr > m_dirtyComposers;

        /** Set to true when something was added or removed form the database */
        bool m_collectionChanged;

        friend class SqlScanResultProcessor;

        // all those classes need to call emptyCache
        friend class TestSqlScanManager;
        friend class TestSqlAlbum;
        friend class TestSqlArtist;
        friend class TestSqlTrack;
        friend class TestSqlCollectionLocation;
};

#endif /* SQLREGISTRY_H */
