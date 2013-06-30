/****************************************************************************************
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

#ifndef AMAROK_FASTFORWARD_WORKER_H
#define AMAROK_FASTFORWARD_WORKER_H

#include "FastForwardImporter.h"

#include "databaseimporter/DatabaseImporter.h"
#include "core/collections/CollectionLocation.h"
#include "core/meta/forward_declarations.h"

#include <threadweaver/Job.h>
#include <threadweaver/ThreadWeaver.h>

#include <QEventLoop>
#include <QSharedPointer>
#include <QSqlDatabase>

class ImporterMiscData
{
    public:
        QString cachedLyrics() const { return m_cachedLyrics; }
        void setCachedLyrics( const QString &lyrics ) { m_cachedLyrics = lyrics; }

        QStringList labels() const { return m_labels; }
        void addLabel( const QString &label );

    private:
        QString m_cachedLyrics;
        QStringList m_labels;
};

class ImporterMiscDataStorage : public QMap<QString, ImporterMiscData>
{
    public:
        void insertCachedLyrics( const QString &url, const QString &lyrics );
        void insertLabel ( const QString &url, const QString &label );
};

class FastForwardWorker : public ThreadWeaver::Job
{
    Q_OBJECT

    public:
        FastForwardWorker();
        ~FastForwardWorker();

        void setDriver( const FastForwardImporter::ConnectionType &driver ) { m_driver = driver; }
        void setDatabaseLocation( const QString &location ) { m_databaseLocation = location; }
        void setDatabase( const QString &name ) { m_database = name; }
        void setHostname( const QString &host ) { m_hostname = host; }
        void setUsername( const QString &user ) { m_username = user; }
        void setPassword( const QString &pass ) { m_password = pass; }
        void setSmartMatch( const bool match ) { m_smartMatch = match; }
        void setImportArtwork( const bool import ) { m_importArtwork = import; }
        void setImportArtworkDir( const QString &dir ) { m_importArtworkDir = dir; }

        bool failed() const { return m_failed; }
        void abort() { m_aborted = true; }

        virtual void run();

    signals:
        void importError( QString );
        void showMessage( QString );
        void trackAdded( Meta::TrackPtr );
        void trackDiscarded( QString );
        void trackMatchFound( Meta::TrackPtr, QString );
        void trackMatchMultiple( Meta::TrackList, QString );

    private slots:
        void resultReady( const Meta::TrackList &tracks );
        void queryDone();

    private:
        QSqlDatabase setupDatabaseConnection();
        const QString driverName() const;
        void failWithError( const QString &errorMsg );

        /**
         * Searches collection by given tags, returns a TrackPtr or 0 if no or multiple tracks match.
         * emits trackDiscarded(), trackMatchFound(), trackMatchMultiple() as appropriate.
         */
        Meta::TrackPtr trySmartMatch( const QString url, const QString title,
                                      const QString album, const QString artist, const QString composer,
                                      const QString genre, const uint year, const uint trackNr,
                                      const uint discNr, const uint filesize );

        /**
         * sets track statistics (score, rating, firstPlayed, lastPlayed, playCount) using
         * StatisticsCapability.
         */
        void setTrackMetadata( Meta::TrackPtr track, double score, int rating, QDateTime firstPlayed,
                                  QDateTime lastPlayed, int playCount );

        /**
         * adds misc track data (lyrics, labels) to cache @param dataForInsert.
         */
        void setTrackMiscData( ImporterMiscDataStorage &dataForInsert, Meta::TrackPtr track,
                               const QString &uniqueId, QString lyrics , QSqlDatabase db );

        /**
         * Inserts misc data such as lyrics and labels to database.
         */
        void insertMiscData( const ImporterMiscDataStorage &dataForInsert );

        /**
         * Copies artwork from A1.4 appdata to A2 appdata
         */
        void importArtwork();

        bool m_aborted;
        bool m_failed;

        FastForwardImporter::ConnectionType m_driver;
        QString m_databaseLocation;
        QString m_database;
        QString m_hostname;
        QString m_username;
        QString m_password;
        bool m_smartMatch;
        bool m_importArtwork;
        QString m_importArtworkDir;

        QMap<QString, QSharedPointer<Collections::CollectionLocation> > m_collectionFolders;
        Meta::TrackList m_matchTracks;
        QEventLoop *m_eventLoop;
};

#endif // multiple inclusion guard
