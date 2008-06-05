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

Mp3tunesWorker::Mp3tunesWorker( Mp3tunesLocker* locker ) : ThreadWeaver::Job()
{
    m_locker = locker;
    connect( this, SIGNAL( done( ThreadWeaver::Job* ) ), SLOT( completeJob() ) );
}

Mp3tunesWorker::~Mp3tunesWorker()
{
}

Mp3tunesLoginWorker::Mp3tunesLoginWorker( Mp3tunesLocker* locker, QString username, QString password ) : Mp3tunesWorker( locker )
{
    m_username = username;
    m_password = password;
    m_sessionId = QString();
}

Mp3tunesLoginWorker::~Mp3tunesLoginWorker()
{
}

void Mp3tunesLoginWorker::run()
{
    if(!m_locker) {
        m_sessionId = m_locker->login(m_username, m_password);
    }
}

void Mp3tunesLoginWorker::completeJob()
{
    emit( finishedLogin( m_sessionId ) );
    deleteLater();
}
