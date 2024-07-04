/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef AMPACHESERVICECOLLECTION_H
#define AMPACHESERVICECOLLECTION_H

#include <ServiceCollection.h>
#include "AmpacheMeta.h"
#include "core/collections/support/TrackForUrlWorker.h"

class AmpacheTrackForUrlWorker : public Amarok::TrackForUrlWorker
{
    Q_OBJECT
    public:
        AmpacheTrackForUrlWorker( const QUrl &url, const MetaProxy::TrackPtr &track,
                                  const QUrl &server, const QString &sessionId,
                                  ServiceBase *service);
        ~AmpacheTrackForUrlWorker() override;
        void run(ThreadWeaver::JobPointer self = QSharedPointer<ThreadWeaver::Job>(), ThreadWeaver::Thread *thread = nullptr) override;
        void parseTrack( const QByteArray &xml );
    Q_SIGNALS:
        void authenticationNeeded();
    private:
        MetaProxy::TrackPtr m_proxy;
        int m_urlTrackId;
        int m_urlAlbumId;
        int m_urlArtistId;

        Meta::AmpacheTrack *m_urlTrack;
        Meta::AmpacheAlbum *m_urlAlbum;
        Meta::ServiceArtist *m_urlArtist;

        QUrl m_server;
        QString m_sessionId;

        ServiceBase *m_service;
};

namespace Collections {

/**
A collection that dynamically fetches data from a remote location as needed

	@author
*/
class AmpacheServiceCollection : public ServiceCollection
{
    Q_OBJECT

public:
    AmpacheServiceCollection( ServiceBase *service, const QUrl &server,
                              const QString &sessionId );

    ~AmpacheServiceCollection() override;

    QueryMaker *queryMaker() override;

    QString collectionId() const override;
    QString prettyName() const override;

    Meta::TrackPtr trackForUrl( const QUrl &url ) override;
    bool possiblyContainsTrack( const QUrl &url ) const override;

Q_SIGNALS:
    void authenticationNeeded();

public Q_SLOTS:
    void slotAuthenticationNeeded();
    void slotLookupComplete( const Meta::TrackPtr & );

private:
    QUrl m_server;
    QString m_sessionId;

    AmpacheTrackForUrlWorker *m_trackForUrlWorker;
};

} //namespace Collections

#endif
