/***************************************************************************
 *   Copyright (c) 2007  Casey Link <unnamedrambler@gmail.com>             *
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

#include "Mp3tunesWorkers.h"

#include "Mp3tunesMeta.h"
#include "Debug.h"

#include <QStringList>

Mp3tunesLoginWorker::Mp3tunesLoginWorker( Mp3tunesLocker* locker, const QString & username, const QString & password ) : ThreadWeaver::Job()
{
    DEBUG_BLOCK
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( completeJob() ) );
    m_locker = locker;
    debug() << "Login Request: " << username << ":" << password;
    m_username = username;
    m_password = password;
    m_sessionId = QString();
}

Mp3tunesLoginWorker::~Mp3tunesLoginWorker()
{
}

void Mp3tunesLoginWorker::run()
{
    DEBUG_BLOCK
    if(m_locker != 0) {
        debug() << "Calling Locker login..";
        m_sessionId = m_locker->login(m_username, m_password);
        debug() << "Login Complete. SessionId = " << m_sessionId;
    } else {
        debug() << "Locker is NULL";
    }
}

void Mp3tunesLoginWorker::completeJob()
{
    DEBUG_BLOCK
    debug() << "Login Job complete";
    emit( finishedLogin( m_sessionId ) );
    deleteLater();
}

Mp3tunesArtistFetcher::Mp3tunesArtistFetcher( Mp3tunesLocker * locker )
{
    m_locker = locker;
}

Mp3tunesArtistFetcher::~Mp3tunesArtistFetcher()
{
}

void Mp3tunesArtistFetcher::run()
{
    DEBUG_BLOCK
    if(m_locker != 0) {
        debug() << "Artist Fetch Start";
        QList<Mp3tunesLockerArtist> list = m_locker->artists();
        debug() << "Artist Fetch End. Total artists: " << list.count();
        m_artists = list;
    } else {
        debug() << "Locker is NULL";
    }
}

void Mp3tunesArtistFetcher::completeJob()
{
    DEBUG_BLOCK
    debug() << "Artist fetch Job complete";
    emit( artistsFetched( m_artists ) );
    deleteLater();
}
