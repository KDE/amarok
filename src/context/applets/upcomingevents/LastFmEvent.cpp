/****************************************************************************************
* Copyright (c) 2009 Nathan Sala <sala.nathan@gmail.com>                               *
* Copyright (c) 2009 Ludovic Deveaux <deveaux.ludovic31@gmail.com>                     *
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

#include "LastFmEvent.h"

int LastFmEvent::metaTypeRegistered = 0;

/**
 * Creates an empty LastFmEvent
 */
LastFmEvent::LastFmEvent() {}

/**
 * Creates a new LastFmEvent with the 'event' attributes
 */
LastFmEvent::LastFmEvent( const LastFmEvent& event)
{
    foreach( QString currentArtist, event.m_artists )
    {
        m_artists.append(currentArtist);    
    }
    m_name = event.m_name;
    m_date = event.m_date;
    m_smallImageUrl = event.m_smallImageUrl;
    m_url = event.m_url;
}

/**
 * Destroys a LastFmEvent instance
 */
LastFmEvent::~LastFmEvent() {}

LastFmEvent::LastFmEvent(QStringList artists, QString name, QDate date, KUrl smallImageUrl, KUrl url)
    : m_artists(artists), m_name(name), m_date(date), m_smallImageUrl(smallImageUrl), m_url(url)
{
    if (!LastFmEvent::metaTypeRegistered)
    {
        qRegisterMetaType<LastFmEvent>("LastFmEvent");
        LastFmEvent::metaTypeRegistered++;
    }
}

QStringList LastFmEvent::artists() const
{
    return m_artists;
}

QDate LastFmEvent::date() const
{
    return m_date;
}

QString LastFmEvent::name() const
{
    return m_name;
}

KUrl LastFmEvent::smallImageUrl() const
{
    return m_smallImageUrl;
}

KUrl LastFmEvent::url() const
{
    return m_url;
}
