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

#include "CollectionManager.h"

#include <QFileInfo>
#include <QList>
#include <QMap>
#include <QPair>
#include <QString>
#include <QStringList>
#include <QVariant>

class SqlCollection;

class ScanResultProcessor : public QObject
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

    signals:
        void changedTrackUrlsUids( const ChangedTrackUrls &, const TrackUrls & ); //not really track urls

    private:
        void addTrack( const QVariantMap &trackData, int albumArtistId );

        int artistId( const QString &artist );
        int artistInsert( const QString &artist );
        int genreId( const QString &genre );
        int genreInsert( const QString &genre );
        int composerId( const QString &composer );
        int composerInsert( const QString &composer );
        int yearId( const QString &year );
        int yearInsert( const QString &year );
        void databaseIdFetch( const QString &artist, const QString &genre, const QString &composer, const QString &year, const QString &album, int artistId );
        int imageId( const QString &image, int albumId );
        int albumId( const QString &album, int artistId );
        int albumInsert( const QString &album, int artistId );
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

    private:
        SqlCollection *m_collection;
        bool m_setupComplete;

        QMap<QString, int> m_artists;
        QMap<QString, int> m_genre;
        QMap<QString, int> m_year;
        QMap<QString, int> m_composer;
        QMap<QPair<QString, int>, int> m_albums;
        QMap<QPair<QString, int>, int> m_images;
        QMap<QString, int> m_directories;
        QMap<QString, QList< QPair< QString, QString > > > m_imageMap;

        QHash<QString, uint> m_filesInDirs;

        TrackUrls m_changedUids; //not really track urls
        ChangedTrackUrls m_changedUrls;

        ScanType m_type;

        QStringList m_aftPermanentTablesUrlString;
        QMap<QString, QString> m_permanentTablesUrlUpdates;
        QMap<QString, QString> m_permanentTablesUidUpdates;
};

#endif
