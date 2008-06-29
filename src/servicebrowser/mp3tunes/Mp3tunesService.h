/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef MP3TUNESSERVICE_H
#define MP3TUNESSERVICE_H

#include "../ServiceBase.h"
#include "Mp3tunesServiceCollection.h"
#include "Mp3tunesLocker.h"

#include <kio/jobclasses.h>
#include <kio/job.h>


class Mp3tunesServiceFactory: public ServiceFactory
{
    Q_OBJECT

    public:
        explicit Mp3tunesServiceFactory() {}
        virtual ~Mp3tunesServiceFactory() {}

        virtual bool possiblyContainsTrack( const KUrl &url ) const;

        virtual void init();
        virtual QString name();
        virtual KPluginInfo info();
        virtual KConfigGroup config();
};


/**
A service for displaying, previewing and downloading music from Mp3tunes.com

	@author 
*/
class Mp3tunesService : public ServiceBase
{
Q_OBJECT

public:
    explicit Mp3tunesService( const QString &name, const QString &email = QString(), const QString &password = QString() );

    ~Mp3tunesService();

    void polish();

    virtual Collection * collection() { return m_collection; }

private slots:
    void authenticate( const QString & uname = "", const QString & passwd = "" );
    void authenticationComplete(  const QString & sessionId );

private:
    QString m_email;
    QString m_password;
    KIO::StoredTransferJob *m_xmlDownloadJob;
    QString m_partnerToken;
    QString m_apiOutputFormat;

    bool m_authenticated;
    QString m_sessionId;

    Mp3tunesServiceCollection *  m_collection;

    Mp3tunesLocker * m_locker;
};

#endif
