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

#ifndef AMPACHESERVICECOLLECTION_H
#define AMPACHESERVICECOLLECTION_H

#include <ServiceCollection.h>
#include "AmpacheMeta.h"
#include "TrackForUrlWorker.h"

#include <kio/jobclasses.h>

class AmpacheTrackForUrlWorker : public Amarok::TrackForUrlWorker
{
    Q_OBJECT
    public:
        AmpacheTrackForUrlWorker( const KUrl &url, MetaProxy::TrackPtr track, const QString &server, const QString &sessionId, ServiceBase* service);
        ~AmpacheTrackForUrlWorker();
        virtual void run ();
        void parseTrack( const QString &xml );
    signals:
        void authenticationNeeded();
    private:
        MetaProxy::TrackPtr mProxy;
        int m_urlTrackId;
        int m_urlAlbumId;
        int m_urlArtistId;

        Meta::AmpacheTrack *m_urlTrack;
        Meta::AmpacheAlbum *m_urlAlbum;
        Meta::ServiceArtist *m_urlArtist;

        QString m_server;
        QString m_sessionId;

        ServiceBase* m_service;

        KIO::StoredTransferJob * m_storedTransferJob;
};


/**
A collection that dynamically fetches data from a remote location as needed

	@author
*/
class AmpacheServiceCollection : public ServiceCollection
{
    Q_OBJECT

public:
    AmpacheServiceCollection( ServiceBase * service, const QString &server, const QString &sessionId );

    virtual ~AmpacheServiceCollection();

    virtual QueryMaker* queryMaker();

    virtual QString collectionId() const;
    virtual QString prettyName() const;

    virtual Meta::TrackPtr trackForUrl( const KUrl &url );
    virtual bool possiblyContainsTrack( const KUrl &url ) const;

signals:
    void authenticationNeeded();

public slots:
    void slotAuthenticationNeeded();
    void slotLookupComplete( const Meta::TrackPtr& );

private:
    /*void parseAlbum( const QString &xml );
    void parseArtist( const QString &xml );*/

    QString m_server;
    QString m_sessionId;

    AmpacheTrackForUrlWorker * m_trackForUrlWorker;
};
#endif
