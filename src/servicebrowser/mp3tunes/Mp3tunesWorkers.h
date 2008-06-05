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
#ifndef MP3TUNESWORKERS_H
#define MP3TUNESWORKERS_H

#include "Mp3tunesLocker.h"

#include <QString>

#include <threadweaver/Job.h>

class Mp3tunesLoginWorker :  public ThreadWeaver::Job
{
    Q_OBJECT
    public:
        Mp3tunesLoginWorker( Mp3tunesLocker* locker, QString username, QString password );
        ~Mp3tunesLoginWorker();

        void run();

    signals:
        void finishedLogin( QString sessionId );

    private slots:
        void completeJob();

    private:
        Mp3tunesLocker* m_locker;
        QString m_sessionId;
        QString m_username;
        QString m_password;
};

#endif
