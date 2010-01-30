/****************************************************************************************
 * Copyright (c) 2009 Nathan Sala <sala.nathan@gmail.com>                               *
 * Copyright (c) 2009-2010 Ludovic Deveaux <deveaux.ludovic31@gmail.com>                *
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

#ifndef LASTFMEVENT_H
#define LASTFMEVENT_H

#include <QString>
#include <QDateTime>
#include <QStringList>
#include <KUrl>

/**
 * A class to store an event fetched from the last.fm API
 * by the request artist.getEvents
 */
class LastFmEvent
{

private:
    QStringList m_artists;      //The list of the participants of the event
    QString m_name;             //The event's name
    QString m_location;         //The location where the event will take place
    QDateTime m_date;           //The event's date
    KUrl m_smallImageUrl;       //The URL to the event's image
    KUrl m_url;                 //The URL to the event's page

public:

    typedef QList< LastFmEvent > LastFmEventList;

    /**
     * Creates an empty LastFmEvent
     */
    LastFmEvent();

    /**
     * Creates a copy of a LastFmEvent
     * @param event the event to be copied from
     */
    LastFmEvent( const LastFmEvent& );

    /**
     * Destroys a LastFmEvent instance
     */
    ~LastFmEvent();

    /**
     * A getter for the artists list
     * @return the list of the participants of the event
     */
    QStringList artists() const;

    /**
     * A getter for the event's name
     * @return the event's name
     */
    QString name() const;

    /**
     * A getter for the event's date
     * @return the event's date
     */
    QDateTime date() const;

    /**
     * A getter for the event's location
     * @return the event's location
     */
    QString location() const;

    /**
     * A getter for the event's image
     * @return the URL to the event's image
     */
    KUrl smallImageUrl() const;

    /**
     * A getter for the event's page
     * @return the URL to the event's page
     */
    KUrl url() const;

    /**
     * Sets the event's artists
     * @param artists the list of the participants of the event
     */
    void setArtists( const QStringList artists );

    /**
     * Sets the event's name
     * @param name the event's name
     */
    void setName( const QString name );

    /**
     * Sets the event's date
     * @param date the event's date
     */
    void setDate( const QDateTime date );

    /**
     * Sets the event's location
     * @param location the event's location
     */
    void setLocation( const QString location );

    /**
     * Sets the event's image
     * @param smallImageUrl the URL to the event's image
     */
    void setSmallImageUrl( const KUrl smallImageUrl );

    /**
     * Sets the event's page
     * @param url the URL to the event's page
     */
    void setUrl( const KUrl url );
    
};

#endif // LASTFMEVENT_H

Q_DECLARE_METATYPE(LastFmEvent)
Q_DECLARE_METATYPE(LastFmEvent::LastFmEventList)
