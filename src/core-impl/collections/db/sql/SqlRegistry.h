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
#include "MountPointManager.h"
#include "amarok_sqlcollection_export.h"

#include <QHash>
#include <QMutex>
#include <QObject>
#include <QPair>
#include <QTimer>
#include <QList>

class SqlStorage;

class TestScanResultProcessorFull;
class TestSqlAlbum;
class TestSqlArtist;
class TestSqlTrack;
class TestSqlCollectionLocation;

namespace Collections {
    class SqlCollection;
    class SqlQueryMakerInternal;
}
typedef QPair<int, QString> TrackId;

/** The SqlRegistry class buffers Meta objects from an Sql database.
 *  This class can be considered a memory cache for the Sql database.
 *  All requests for Meta objects like SqlTrack should go through here.
 */
class AMAROK_SQLCOLLECTION_EXPORT_TESTS SqlRegistry : public QObject
{
    Q_OBJECT

    public:
        SqlRegistry(Collections::SqlCollection *collection);
        virtual ~SqlRegistry();

        /** Searches a directory entry in the scanned directories
            This function searches an exisiting directory entry.
            @param mtime if mtime is != 0 then the mtime of the entry is updated
            @returns the directory id
         */
        int getDirectory( const QString &path, uint mtime = 0 );

        Meta::TrackPtr getTrack( int id );
        Meta::TrackPtr getTrack( const QString &path );

        /** Returns the track located at the given url or a new one if not existing.
            This is kind of dangerous because it can generate a new track in
            the database without a file on the filesystem, so don't call it unless
            you really want tracks to be generate.
            The new track must be commited by writing
            some other meta information.
            Use SqlCollection::trackForUrl instead.
         */
        Meta::TrackPtr getTrack( int deviceId, const QString &rpath, int directoryId, const QString &uidUrl );
        Meta::TrackPtr getTrackFromUid( const QString &uid );

        /** Removes the track from the database (but not from the file system) */
        void deleteTrack( int trackId );

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

    private slots:
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
            @return true if the update was successfull.
        */
        bool updateCachedUrl( const QString &oldUrl, const QString &newUrl );

        /** Updates the uid of an already cached track.
            @return true if the update was successfull.
        */
        bool updateCachedUid( const QString &oldUid, const QString &newUid );

        friend class Meta::SqlTrack;

        // only the query maker creates Metas like this
        Meta::TrackPtr getTrack( int id, const QStringList &rowData );
        Meta::ArtistPtr getArtist( int id, const QString &name );
        Meta::GenrePtr getGenre( int id, const QString &name );
        Meta::ComposerPtr getComposer( int id, const QString &name );
        Meta::AlbumPtr getAlbum( int id, const QString &album, int artistId );
        Meta::LabelPtr getLabel( int id, const QString &label );

        friend class Collections::SqlQueryMakerInternal;

        //we don't care about the ordering so use the faster QHash
        QHash<TrackId, Meta::TrackPtr > m_trackMap;
        QHash<QString, Meta::TrackPtr > m_uidMap;
        QHash<QString, Meta::ArtistPtr > m_artistMap;
        QHash<int, Meta::ArtistPtr > m_artistIdMap;
        QHash<QString, Meta::ComposerPtr > m_composerMap;
        QHash<QString, Meta::GenrePtr > m_genreMap;
        QHash<int, Meta::YearPtr > m_yearMap;
        QHash<AlbumKey, Meta::AlbumPtr > m_albumMap;
        QHash<int, Meta::AlbumPtr > m_albumIdMap;
        QHash<QString, Meta::LabelPtr > m_labelMap;

        QMutex m_trackMutex;
        QMutex m_artistMutex;
        QMutex m_composerMutex;
        QMutex m_genreMutex;
        QMutex m_yearMutex;
        QMutex m_albumMutex;
        QMutex m_labelMutex;

        QTimer *m_timer;

        Collections::SqlCollection *m_collection;

        // all those classes need to call emptyCache
        friend class TestScanResultProcessorFull;
        friend class TestSqlAlbum;
        friend class TestSqlArtist;
        friend class TestSqlTrack;
        friend class TestSqlCollectionLocation;
};

#endif /* SQLREGISTRY_H */
