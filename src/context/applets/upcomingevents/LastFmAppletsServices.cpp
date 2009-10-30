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

#include "LastFmAppletsServices.h"
#include "LastFmEvent.h"

#include <KDebug>
#include <QStringList>
#include <lastfm/Artist>
#include <lastfm/XmlQuery>
#include <lastfm/ws.h>


/*void LastFmAppletsServices::similarArtistsFetched()
{
    QList< lastfm::Artist > similarArtists;
    QByteArray result = reply->readAll();
    kDebug()<< result;
    
    emit readyToDisplaySimilarArtists(similarArtists&);
}

void LastFmAppletsServices::sendSimilarArtistsRequest(const QString& artist_name)
{
    lastfm::Artist artist(artist_name);
    m_reply = artist.getSimilar();
    connect(m_reply, SIGNAL(finished()), this, SLOT(similarArtistsFetched()));
}*/

QList<LastFmEvent> LastFmAppletsServices::upcomingEvents(const QString &artist_name) {

    //Initialize the query parameters
    QMap< QString, QString > params;
    params["method"] = "getEvents";
    params["artist"] = artist_name;
    
    m_reply = lastfm::ws::get(params);
    QList<LastFmEvent> events;
    try
    {
        //Parse the XML reply
        lastfm::XmlQuery xml = lastfm::ws::parse(m_reply);
        foreach (lastfm::XmlQuery xmlEvent, xml.children("event"))
        {
            QStringList artists;
            foreach (lastfm::XmlQuery xmlArtists, xmlEvent["artists"])
            {
                artists.append(xmlArtists.text());
            }
            QString title = xmlEvent["title"].text();
            QString date = xmlEvent["startDate"].text();
            LastFmEvent event(artists, title, date);
            events.append(event);
        }
        return events;
    }
    catch (lastfm::ws::ParseError& e)
    {
        kDebug() << e.what();
    }
    //connect(m_reply, SIGNAL( finished() ), SLOT( eventsFetched() ))
}