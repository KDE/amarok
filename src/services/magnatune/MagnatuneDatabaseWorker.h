/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef MAGNATUNEDATABASEWORKER_H
#define MAGNATUNEDATABASEWORKER_H

#include "MagnatuneMeta.h"

#include "../ServiceSqlRegistry.h"

#include <ThreadWeaver/Job>

/**
A small helper class to do some simple asynchronous database queries

	@author Nikolaj Hald Nielsen <nhn@kde.org>   
*/

class MagnatuneDatabaseWorker : public QObject, public ThreadWeaver::Job
{
    Q_OBJECT
public:
    MagnatuneDatabaseWorker();

    ~MagnatuneDatabaseWorker();

    void run(ThreadWeaver::JobPointer self = QSharedPointer<ThreadWeaver::Job>(), ThreadWeaver::Thread *thread = 0) override;

    void fetchMoodMap();
    void fetchTrackswithMood( const QString &mood, int noOfTracks, ServiceSqlRegistry * registry );
    void fetchAlbumBySku( const QString &sku, ServiceSqlRegistry * registry );

Q_SIGNALS:
    /** This signal is emitted when this job is being processed by a thread. */
    void started(ThreadWeaver::JobPointer);
    /** This signal is emitted when the job has been finished (no matter if it succeeded or not). */
    void done(ThreadWeaver::JobPointer);
    /** This job has failed.
     * This signal is emitted when success() returns false after the job is executed. */
    void failed(ThreadWeaver::JobPointer);

    void gotMoodMap( const QMap<QString, int> &map );
    void gotMoodyTracks( Meta::TrackList tracks );
    void gotAlbumBySku( Meta::MagnatuneAlbum * album );

private Q_SLOTS:
    void completeJob();

private:

    void doFetchMoodMap();
    void doFetchTrackswithMood();
    void doFetchAlbumBySku();
    
    enum taskType { FETCH_MODS, FETCH_MOODY_TRACKS, ALBUM_BY_SKU };

    int m_task;

    QMap<QString, int> m_moodMap;
    Meta::TrackList m_moodyTracks;

    QString m_mood;
    QString m_sku;
    int m_noOfTracks;
    Meta::MagnatuneAlbum * m_album;

    ServiceSqlRegistry * m_registry;

protected:
    void defaultBegin(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
    void defaultEnd(const ThreadWeaver::JobPointer& job, ThreadWeaver::Thread *thread) override;
};

#endif
