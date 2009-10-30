/****************************************************************************************
* Copyright (c) 2009 Nathan Sala <sala.nathan@gmail.com>                               *
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

#ifndef LASTFMAPPLETSSERVICES_H
#define LASTFMAPPLETSSERVICES_H

#include <QMutex>

#include <lastfm/Audioscrobbler>
#include <lastfm/NetworkAccessManager>
#include <lastfm/XmlQuery>

class LastFmAppletsServices
{
    Q_OBJECT

private:
    QNetworkReply *m_reply;
    QMutex m_mutex;

/*private slots:
    void similarArtistsFetched();

public signals:
    void readyToDisplaySimilarArtists(QList<lastfm::Artist> &similarArtists);
    
public:
    void sendSimilarArtistsRequest(const QString &artist_name);*/

public:
    QList<LastFmEvent> upcomingEvents(const QString &artist_name);
};

#endif // LASTFMAPPLETSSERVICES_H
