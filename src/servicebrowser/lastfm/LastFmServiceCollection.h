/***************************************************************************
 * copyright            : (C) 2008 Shane King <kde@dontletsstart.com>      *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef LASTFMSERVICECOLLECTION_H
#define LASTFMSERVICECOLLECTION_H

#include <ServiceCollection.h>

#include "WeightedStringList.h"

class Request;

namespace Meta
{
    class ServiceGenre;
}

class LastFmServiceCollection : public ServiceCollection
{
    Q_OBJECT;
public:
    LastFmServiceCollection( const QString& userName );
    virtual ~LastFmServiceCollection();

    virtual bool possiblyContainsTrack( const KUrl &url ) const;
    virtual Meta::TrackPtr trackForUrl( const KUrl &url );

    virtual QString collectionId() const;
    virtual QString prettyName() const;

    virtual QueryMaker* queryMaker();

private slots:
    void slotAddNeighbours( WeightedStringList list );
    void slotAddFriends( QStringList list );
    void slotRecentlyLovedTrackResult( Request* );
    void slotRecentTrackResult( Request* );

private:
    QString m_userName;
    Meta::ServiceGenre *m_neighbors;
    Meta::ServiceGenre *m_friends;
    Meta::ServiceGenre *m_recentlyLoved;
    Meta::ServiceGenre *m_recentlyPlayed;
};

#endif // LASTFMSERVICECOLLECTION_H
