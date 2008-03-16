/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
 
#include "MagnatuneDatabaseWorker.h"

#include "CollectionManager.h"
#include "SqlStorage.h"

MagnatuneDatabaseWorker::MagnatuneDatabaseWorker()
    : ThreadWeaver::Job()
{
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( completeJob() ) );
}


MagnatuneDatabaseWorker::~MagnatuneDatabaseWorker()
{
}


void
MagnatuneDatabaseWorker::run()
{
    switch ( m_task ) {
        case FETCH_MODS:
            doFetchMoodMap();
            break;
        case FETCH_MOODY_TRACKS:
            doFetchTrackswithMood();
            break;
        default:
            break;
    }
}

void MagnatuneDatabaseWorker::completeJob()
{
    switch ( m_task ) {
        case FETCH_MODS:
            emit( gotMoodMap( m_moodMap ) );
            break;
        case FETCH_MOODY_TRACKS:
            doFetchTrackswithMood();
            break;
        default:
            break;
    }
    deleteLater();
}




void MagnatuneDatabaseWorker::fetchMoodMap()
{
    m_task = FETCH_MODS;
    m_moodMap.clear();
}

void MagnatuneDatabaseWorker::fetchTrackswithMood(QString mood, int noOfTracks)
{
    m_task = FETCH_MOODY_TRACKS;
    m_mood = mood;
    m_noOfTracks = noOfTracks;

    m_moodyTracks.clear();
}


void MagnatuneDatabaseWorker::doFetchMoodMap()
{
    SqlStorage *sqlDb = CollectionManager::instance()->sqlStorage();
    QString queryString = "select count( mood ), mood from magnatune_moods GROUP BY mood;";
    debug() << "Querying for moodss: " << queryString;
    QStringList result = sqlDb->query( queryString );
    debug() << "result: " << result;

    while ( !result.isEmpty() ) {
        int count = result.takeFirst().toInt();
        QString string =  result.takeFirst();
        m_moodMap.insert( string, count );
    }

}

void MagnatuneDatabaseWorker::doFetchTrackswithMood()
{
    SqlStorage *sqlDb = CollectionManager::instance()->sqlStorage();
}

#include "MagnatuneDatabaseWorker.moc"

