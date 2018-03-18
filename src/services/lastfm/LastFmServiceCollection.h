/****************************************************************************************
 * Copyright (c) 2008 Shane King <kde@dontletsstart.com>                                *
 * Copyright (c) 2008 Leo Franchi <lfranchi@gmail.com>                                  *
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

#ifndef LASTFMSERVICECOLLECTION_H
#define LASTFMSERVICECOLLECTION_H

#include "services/ServiceCollection.h"

class QNetworkReply;

namespace Meta
{
    class ServiceGenre;
}

namespace Collections {

class LastFmServiceCollection : public ServiceCollection
{
    Q_OBJECT
public:
    explicit LastFmServiceCollection( const QString &userName );
    virtual ~LastFmServiceCollection();

    virtual bool possiblyContainsTrack( const QUrl &url ) const;
    virtual Meta::TrackPtr trackForUrl( const QUrl &url );

    virtual QString collectionId() const;
    virtual QString prettyName() const;

    virtual QueryMaker* queryMaker();

private Q_SLOTS:
    void slotAddNeighboursLoved();
    void slotAddNeighboursPersonal();
    void slotAddFriendsLoved();
    void slotAddFriendsPersonal();
    
private:
    QMap< QString, QNetworkReply* > m_jobs;
    Meta::ServiceGenre *m_neighborsLoved;
    Meta::ServiceGenre *m_neighborsPersonal;
    Meta::ServiceGenre *m_friendsLoved;
    Meta::ServiceGenre *m_friendsPersonal;
};

} //namespace Collections

#endif // LASTFMSERVICECOLLECTION_H
