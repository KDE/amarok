/****************************************************************************************
 * Copyright (c) 2007,2008 Casey Link <unnamedrambler@gmail.com>                        *
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

#ifndef MP3TUNESWORKERS_H
#define MP3TUNESWORKERS_H

#include "Mp3tunesLocker.h"

#include <QList>
#include <QStringList>

#include <ThreadWeaver/Job>

/**
 * Allows for threading the logging in process.
 */
class Mp3tunesLoginWorker : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT
    public:
        Mp3tunesLoginWorker( Mp3tunesLocker* locker, const QString &username, const QString &password );
        ~Mp3tunesLoginWorker();

        void run(ThreadWeaver::JobPointer self = QSharedPointer<ThreadWeaver::Job>(), ThreadWeaver::Thread *thread = 0) override;

    Q_SIGNALS:
        /** This signal is emitted when this job is being processed by a thread. */
        void started(ThreadWeaver::JobPointer);
        /** This signal is emitted when the job has been finished (no matter if it succeeded or not). */
        void done(ThreadWeaver::JobPointer);
        /** This job has failed.
         * This signal is emitted when success() returns false after the job is executed. */
        void failed(ThreadWeaver::JobPointer);

        void finishedLogin( const QString &sessionId );

    private Q_SLOTS:
        void completeJob();

    private:
        Mp3tunesLocker* m_locker;
        QString m_sessionId;
        QString m_username;
        QString m_password;
    protected:
        void defaultBegin(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
        void defaultEnd(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
};

/**
 * Allows for threading the artist fetching process
 */
class Mp3tunesArtistFetcher : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT
    public:
        explicit Mp3tunesArtistFetcher( Mp3tunesLocker * locker );
        ~Mp3tunesArtistFetcher();

        void run(ThreadWeaver::JobPointer self = QSharedPointer<ThreadWeaver::Job>(), ThreadWeaver::Thread *thread = 0) override;

    Q_SIGNALS:
        /** This signal is emitted when this job is being processed by a thread. */
        void started(ThreadWeaver::JobPointer);
        /** This signal is emitted when the job has been finished (no matter if it succeeded or not). */
        void done(ThreadWeaver::JobPointer);
        /** This job has failed.
         * This signal is emitted when success() returns false after the job is executed. */
        void failed(ThreadWeaver::JobPointer);

        void artistsFetched( QList<Mp3tunesLockerArtist> );

    private Q_SLOTS:
        void completeJob();

    private:
        Mp3tunesLocker* m_locker;
        QList<Mp3tunesLockerArtist> m_artists;
    protected:
        void defaultBegin(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
        void defaultEnd(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
};

/**
 * Allows for threading the albumWithArtistId fetching process
 */
class Mp3tunesAlbumWithArtistIdFetcher : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT
    public:
        Mp3tunesAlbumWithArtistIdFetcher( Mp3tunesLocker * locker, int artistId );
        ~Mp3tunesAlbumWithArtistIdFetcher();

        void run(ThreadWeaver::JobPointer self = QSharedPointer<ThreadWeaver::Job>(), ThreadWeaver::Thread *thread = 0) override;

    Q_SIGNALS:
        /** This signal is emitted when this job is being processed by a thread. */
        void started(ThreadWeaver::JobPointer);
        /** This signal is emitted when the job has been finished (no matter if it succeeded or not). */
        void done(ThreadWeaver::JobPointer);
        /** This job has failed.
         * This signal is emitted when success() returns false after the job is executed. */
        void failed(ThreadWeaver::JobPointer);

        void albumsFetched( QList<Mp3tunesLockerAlbum> );

    private Q_SLOTS:
        void completeJob();

    private:
        int m_artistId;
        Mp3tunesLocker* m_locker;
        QList<Mp3tunesLockerAlbum> m_albums;
    protected:
        void defaultBegin(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
        void defaultEnd(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
};

/**
 * Allows for threading the trackWithAlbumId fetching process
 */
class Mp3tunesTrackWithAlbumIdFetcher : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT
    public:
        Mp3tunesTrackWithAlbumIdFetcher( Mp3tunesLocker * locker, int albumId );
        ~Mp3tunesTrackWithAlbumIdFetcher();

        void run(ThreadWeaver::JobPointer self = QSharedPointer<ThreadWeaver::Job>(), ThreadWeaver::Thread *thread = 0) override;

    Q_SIGNALS:
        /** This signal is emitted when this job is being processed by a thread. */
        void started(ThreadWeaver::JobPointer);
        /** This signal is emitted when the job has been finished (no matter if it succeeded or not). */
        void done(ThreadWeaver::JobPointer);
        /** This job has failed.
         * This signal is emitted when success() returns false after the job is executed. */
        void failed(ThreadWeaver::JobPointer);

        void tracksFetched( QList<Mp3tunesLockerTrack> );

    private Q_SLOTS:
        void completeJob();

    private:
        int m_albumId;
        Mp3tunesLocker* m_locker;
        QList<Mp3tunesLockerTrack> m_tracks;
    protected:
        void defaultBegin(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
        void defaultEnd(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
};

/**
 * Allows for threading the trackWithArtistId fetching process
 */
class Mp3tunesTrackWithArtistIdFetcher : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT
    public:
        Mp3tunesTrackWithArtistIdFetcher( Mp3tunesLocker * locker, int artistId );
        ~Mp3tunesTrackWithArtistIdFetcher();

        void run(ThreadWeaver::JobPointer self = QSharedPointer<ThreadWeaver::Job>(), ThreadWeaver::Thread *thread = 0) override;

    Q_SIGNALS:
        /** This signal is emitted when this job is being processed by a thread. */
        void started(ThreadWeaver::JobPointer);
        /** This signal is emitted when the job has been finished (no matter if it succeeded or not). */
        void done(ThreadWeaver::JobPointer);
        /** This job has failed.
         * This signal is emitted when success() returns false after the job is executed. */
        void failed(ThreadWeaver::JobPointer);
        void tracksFetched( QList<Mp3tunesLockerTrack> );

    private Q_SLOTS:
        void completeJob();

    private:
        int m_artistId;
        Mp3tunesLocker* m_locker;
        QList<Mp3tunesLockerTrack> m_tracks;
    protected:
        void defaultBegin(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
        void defaultEnd(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;

};

/**
 * Allows for threading the searching process
 */
class Mp3tunesSearchMonkey : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT
    public:
        Mp3tunesSearchMonkey( Mp3tunesLocker * locker, QString query, int searchFor );
        ~Mp3tunesSearchMonkey();

        void run(ThreadWeaver::JobPointer self = QSharedPointer<ThreadWeaver::Job>(), ThreadWeaver::Thread *thread = 0) override;

    Q_SIGNALS:
        /** This signal is emitted when this job is being processed by a thread. */
        void started(ThreadWeaver::JobPointer);
        /** This signal is emitted when the job has been finished (no matter if it succeeded or not). */
        void done(ThreadWeaver::JobPointer);
        /** This job has failed.
         * This signal is emitted when success() returns false after the job is executed. */
        void failed(ThreadWeaver::JobPointer);

        void searchArtistComplete( QList<Mp3tunesLockerArtist> );
        void searchAlbumComplete( QList<Mp3tunesLockerAlbum> );
        void searchTrackComplete( QList<Mp3tunesLockerTrack> );

    private Q_SLOTS:
        void completeJob();

    private:
        QString m_query;
        int m_searchFor;
        Mp3tunesLocker* m_locker;
        Mp3tunesSearchResult m_result;
    protected:
        void defaultBegin(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
        void defaultEnd(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
};

/**
 * Allows for threading a track list upload
 */
class Mp3tunesSimpleUploader : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT
    public:
        Mp3tunesSimpleUploader( Mp3tunesLocker * locker, QStringList tracklist );
        ~Mp3tunesSimpleUploader();

        void run(ThreadWeaver::JobPointer self = QSharedPointer<ThreadWeaver::Job>(), ThreadWeaver::Thread *thread = 0) override;

    Q_SIGNALS:
        /** This signal is emitted when this job is being processed by a thread. */
        void started(ThreadWeaver::JobPointer);
        /** This signal is emitted when the job has been finished (no matter if it succeeded or not). */
        void done(ThreadWeaver::JobPointer);
        /** This job has failed.
         * This signal is emitted when success() returns false after the job is executed. */
        void failed(ThreadWeaver::JobPointer);

        void uploadComplete();
        void incrementProgress();
        void endProgressOperation( QObject * );

    private Q_SLOTS:
        void completeJob();

    private:
        Mp3tunesLocker* m_locker;
        QStringList m_tracklist;
    protected:
        void defaultBegin(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
        void defaultEnd(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
};

/**
 * Allows for threading a track from filekey job.
 */
class Mp3tunesTrackFromFileKeyFetcher : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT
    public:
        Mp3tunesTrackFromFileKeyFetcher( Mp3tunesLocker * locker, QString filekey );
        ~Mp3tunesTrackFromFileKeyFetcher();

        void run(ThreadWeaver::JobPointer self = QSharedPointer<ThreadWeaver::Job>(), ThreadWeaver::Thread *thread = 0) override;

    Q_SIGNALS:
        /** This signal is emitted when this job is being processed by a thread. */
        void started(ThreadWeaver::JobPointer);
        /** This signal is emitted when the job has been finished (no matter if it succeeded or not). */
        void done(ThreadWeaver::JobPointer);
        /** This job has failed.
         * This signal is emitted when success() returns false after the job is executed. */
        void failed(ThreadWeaver::JobPointer);

        void trackFetched( Mp3tunesLockerTrack &track );

    private Q_SLOTS:
        void completeJob();

    private:
        Mp3tunesLocker* m_locker;
        Mp3tunesLockerTrack m_track;
        QString m_filekey;

    protected:
        void defaultBegin(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
        void defaultEnd(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
};

#endif

