/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
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

#ifndef AMAROK_SQL_SCANRESULTPROCESSOR_H
#define AMAROK_SQL_SCANRESULTPROCESSOR_H

#include "SqlCollection.h"

#include <QFileInfo>
#include <QList>
#include <QMap>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QVariant>

#include "amarok_sqlcollection_export.h"

class SqlStorage;

class AMAROK_SQLCOLLECTION_EXPORT_TESTS ScanResultProcessor : public QObject
{
    Q_OBJECT

    public:
        enum ScanType
        {
            FullScan = 0,
            IncrementalScan = 1
        };

        ScanResultProcessor( SqlCollection *collection );
        ~ScanResultProcessor();

        void addDirectory( const QString &dir, uint mtime );
        void addImage( const QString &path, const QList< QPair<QString, QString> > );
        void setScanType( ScanType type );
        void doneWithImages();
        void processDirectory( const QList<QVariantMap > &data );
        void commit();
        void rollback();

        void setSqlStorage( SqlStorage *storage ) { m_storage = storage; }

    signals:
        void changedTrackUrlsUids( const ChangedTrackUrls &, const TrackUrls & ); //not really track urls

    private:
        void addTrack( const QVariantMap &trackData, int albumArtistId );

        int genericId( QHash<QString, int> *hash, const QString &value, int *currNum );
        int imageId( const QString &image, int albumId );
        int albumId( const QString &album, int albumArtistId );
        int albumInsert( const QString &album, int albumArtistId );
        int urlId( const QString &url, const QString &uid );
        int directoryId( const QString &dir );

        QString findBestImagePath( const QList<QString> &paths );

        //void updateAftPermanentTablesUrlId( int urlId, const QString &uid );
        //void updateAftPermanentTablesUidId( int urlId, const QString &uid );
        void updateAftPermanentTablesUrlString();
        void updateAftPermanentTablesUidString();

        int checkExistingAlbums( const QString &album );

        QString findAlbumArtist( const QSet<QString> &artists, int trackCount ) const;
        void setupDatabase();
        void populateCacheHashes();
        void copyHashesToTempTables();
        void genericCopyHash( const QString &tableName, const QHash<QString, int> *hash, int maxSize );

    private:
        SqlCollection *m_collection;
        SqlStorage *m_storage;
        bool m_setupComplete;

        QHash<QString, int> m_artists;
        QHash<QString, int> m_genres;
        QHash<QString, int> m_years;
        QHash<QString, int> m_composers;
        QHash<QString, int> m_imagesFlat;
        QMap<QPair<QString, int>, int> m_albums;
        QMap<QPair<QString, int>, int> m_images;
        QMap<QString, int> m_directories;
        QMap<QString, QList< QPair< QString, QString > > > m_imageMap;

        QSet<QString> m_uidsSeenThisScan;
        QHash<QString, uint> m_filesInDirs;

        TrackUrls m_changedUids; //not really track urls
        ChangedTrackUrls m_changedUrls;

        ScanType m_type;

        QStringList m_aftPermanentTablesUrlString;
        QMap<QString, QString> m_permanentTablesUrlUpdates;
        QMap<QString, QString> m_permanentTablesUidUpdates;

        int m_nextUrlNum;
        QHash<QString, QStringList*> m_urlsHashByUid;
        QHash<QPair<int, QString>, QStringList*> m_urlsHashByLocation;
        QHash<int, QStringList*> m_urlsHashById;

        int m_nextAlbumNum;
        QHash<QString, QLinkedList<QStringList*> *> m_albumsHashByName;
        QHash<int, QStringList*> m_albumsHashById;

        int m_nextTrackNum;
        QHash<int, QStringList*> m_tracksHashById;
        QHash<int, QStringList*> m_tracksHashByUrl;
        QHash<int, QLinkedList<QStringList*> *> m_tracksHashByAlbum;

        int m_nextArtistNum;
        int m_nextComposerNum;
        int m_nextGenreNum;
        int m_nextImageNum;
        int m_nextYearNum;
};

#endif
