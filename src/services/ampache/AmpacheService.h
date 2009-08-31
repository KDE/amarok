/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMPACHESERVICE_H
#define AMPACHESERVICE_H



#include "../ServiceBase.h"
#include "AmpacheServiceCollection.h"

#include <kio/jobclasses.h>
#include <kio/job.h>


class AmpacheServiceFactory: public ServiceFactory
{
    Q_OBJECT

    public:
        explicit AmpacheServiceFactory() {}
        virtual ~AmpacheServiceFactory() {}

        virtual bool possiblyContainsTrack( const KUrl &url ) const;

        virtual void init();
        virtual QString name();
        virtual KPluginInfo info();
        virtual KConfigGroup config();
};


/**
A service for displaying, previewing and downloading music from Ampache music servers

	@author 
*/
class AmpacheService : public ServiceBase
{

Q_OBJECT
public:
    explicit AmpacheService( AmpacheServiceFactory* parent, const QString &name,
                             const QString &url = QString(), const QString &username = QString(),
                             const QString &password = QString() );

    ~AmpacheService();

    void polish();
    void reauthenticate();

    virtual Amarok::Collection * collection() { return m_collection; }

private slots:
    void authenticate(KJob *job);
    void authenticationComplete(  KJob *job );
    void versionVerify( KJob *job); 

private:
    KIO::StoredTransferJob *m_xmlDownloadJob;
    KIO::StoredTransferJob *m_xmlVersionJob;

    bool m_authenticated;
    QString m_server;
    QString m_username;
    QString m_password;
    QString m_sessionId;
    int m_version;

    AmpacheServiceCollection *  m_collection;

    // Disable copy constructor and assignment
    AmpacheService( const AmpacheService& );
    AmpacheService& operator=( const AmpacheService& );
};

#endif
