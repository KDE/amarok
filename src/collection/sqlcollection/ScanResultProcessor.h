/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef AMAROK_SQL_SCANRESULTPROCESSOR_H
#define AMAROK_SQL_SCANRESULTPROCESSOR_H

#include <QHash>
#include <QList>
#include <QMap>
#include <QPair>
#include <QString>

class SqlCollection;

class ScanResultProcessor
{

    public:
        enum ScanType
        {
            FullScan = 0,
            IncrementalScan = 1
        };

        ScanResultProcessor( SqlCollection *collection );
        ~ScanResultProcessor();

        void processScanResult( const QMap<QString, QHash<QString, QString> > &scanResult );

        void addDirectory( const QString &dir, uint mtime );
        void setScanType( ScanType type );

    private:
        void processDirectory( const QList<QHash<QString, QString> > &data );
        void addTrack( const QHash<QString, QString> &trackData, int albumArtistId );

        int artistId( const QString &artist );
        int genreId( const QString &genre );
        int composerId( const QString &composer );
        int yearId( const QString &year );
        int albumId( const QString &album, int artistId );
        int urlId( const QString &url );
        int directoryId( const QString &dir );

        int checkExistingAlbums( const QString &album );

        QString findAlbumArtist( const QSet<QString> &artists ) const;
        void setupDatabase();

    private:
        SqlCollection *m_collection;
        bool m_setupComplete;

        QMap<QString, int> m_artists;
        QMap<QString, int> m_genre;
        QMap<QString, int> m_year;
        QMap<QString, int> m_composer;
        QMap<QPair<QString, int>, int> m_albums;
        QMap<QString, int> m_directories;

        QHash<QString, uint> m_filesInDirs;

        ScanType m_type;
};

#endif
