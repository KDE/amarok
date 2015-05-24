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

#include <KDateTime>
#include <KSharedPtr>
#include <QUrl>

#include <QSharedData>
#include <QStringList>

class LastFmEvent;
class LastFmLocation;
class LastFmVenue;

typedef KSharedPtr<LastFmEvent> LastFmEventPtr;
typedef KSharedPtr<LastFmVenue> LastFmVenuePtr;
typedef KSharedPtr<LastFmLocation> LastFmLocationPtr;

/**
 * A class to store an event fetched from the last.fm API
 * by the request artist.getEvents
 */
class LastFmEvent : public QSharedData
{
public:
    enum ImageSize
    {
        Small      = 0,
        Medium     = 1,
        Large      = 2,
        ExtraLarge = 3,
        Mega       = 4
    };

    typedef QList< LastFmEventPtr > List;
    typedef QHash<ImageSize, QUrl> ImageUrls;

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
     * A getter for the artists list.
     * It consists of the headliner + participants.
     * @return the list of all artists participating the event
     */
    QStringList artists() const;

    /**
     * The number of people attending the event
     * @return number of people attending the event
     */
    int attendance() const
    { return m_attendance; }

    /**
     * A getter for the event's date
     * @return the event's date
     */
    KDateTime date() const;

    /**
     * The event's description
     * @return event's description
     */
    QString description() const
    { return m_description; }

    /**
     * The event's headlining artist
     * @return event's headlining artist
     */
    QString headliner() const
    { return m_headliner; }

    /**
     * Gets the URL for the event's event at \p size;
     * @param size size of the image
     * @return image URL
     */
    QUrl imageUrl( ImageSize size ) const
    { return m_imageUrls.value(size); }

    /**
     * Whether the event is cancelled
     * @return true if the event is cancelled
     */
    bool isCancelled() const
    { return m_cancelled; }

    /**
     * A getter for the event's name
     * @return the event's name
     */
    QString name() const;

    /**
     * The list of participating artists (excluding the headliner)
     * @return list of participating artists (excluding the headliner)
     */
    QStringList participants() const
    { return m_participants; }

    /**
     * The List of Last.fm tags
     * @return list of Last.fm tags
     */
    QStringList tags() const
    { return m_tags; }

    /**
     * A getter for the event's page
     * @return the URL to the event's page
     */
    QUrl url() const;

    /**
     * Get the venue associated with this event
     * @return the venue
     */
    LastFmVenuePtr venue() const
    { return m_venue; }

    /**
     * Set the number of attendance
     * @param number the number of attendance
     */
    void setAttendance( int number )
    { m_attendance = number; }

    /**
     * Set whether the event has been cancelled
     * @param isCancelled whether the event has been cancelled
     */
    void setCancelled( bool isCancelled )
    { m_cancelled = isCancelled; }

    /**
     * Sets the event's date
     * @param date the event's date
     */
    void setDate( const KDateTime &date );

    /**
     * Sets the event's description
     * @param text the event's description
     */
    void setDescription( const QString &text )
    { m_description = text; }

    /**
     * Sets the headlining artist for this event
     * @param headliner the headlining artist for this event
     */
    void setHeadliner( const QString &headliner )
    { m_headliner = headliner; }

    /**
     * Sets the \p url for the event's image at \p size
     * @param size size of the image
     * @param url url of the image
     */
    void setImageUrl( ImageSize size, const QUrl &url )
    { m_imageUrls[size] = url; }

    /**
     * Sets the event's name
     * @param name the event's name
     */
    void setName( const QString &name );

    /**
     * Sets the participating artists (excluding headliner) at this event
     * @param participants artists participating at this event
     */
    void setParticipants( const QStringList &participants )
    { m_participants = participants; }

    /**
     * Sets the tags for this event
     * @param tags the tags for this event
     */
    void setTags( const QStringList &tags )
    { m_tags = tags; }

    /**
     * Sets the event's page
     * @param url the URL to the event's page
     */
    void setUrl( const QUrl &url );

    /**
     * Sets the venue of this event
     * @param venue the venue of this event
     */
    void setVenue( LastFmVenuePtr venue ) { m_venue = venue; }

    /**
     * Convert an ImageSize to a QString
     */
    static QString imageSizeToString( ImageSize size );

    /**
     * Convert a QString to an ImageSize
     */
    static ImageSize stringToImageSize( const QString &string );

private:
    int m_attendance;            //!< Number of the event's attendance
    bool m_cancelled;            //!< Whether the event has been cancelled
    KDateTime m_date;            //!< The event's start date
    QUrl m_url;                  //!< The URL to the event's page
    ImageUrls m_imageUrls;       //!< URLs to the event's image
    QString m_description;       //!< Description of the event
    QString m_name;              //!< The event's name
    QString m_headliner;         //!< The headline artist of this event
    QStringList m_participants;  //!< Other artists participating in the event
    QStringList m_tags;          //!< Contextual tags
    LastFmVenuePtr m_venue;      //!< Venue info
};

class LastFmLocation : public QSharedData
{
public:
    LastFmLocation();
    ~LastFmLocation();
    LastFmLocation( const LastFmLocation &cpy );

    QString city;
    QString country;
    QString street;
    QString postalCode;
    double latitude;
    double longitude;
};

class LastFmVenue : public QSharedData
{
public:
    LastFmVenue();
    ~LastFmVenue();
    LastFmVenue( const LastFmVenue &cpy );

    int id;
    QString name;
    QUrl url;
    QUrl website;
    QString phoneNumber;
    QHash<LastFmEvent::ImageSize, QUrl> imageUrls;
    LastFmLocationPtr location;
};

Q_DECLARE_METATYPE(LastFmEvent)
Q_DECLARE_METATYPE(LastFmEventPtr)
Q_DECLARE_METATYPE(LastFmEvent::List)
Q_DECLARE_METATYPE(LastFmLocation)
Q_DECLARE_METATYPE(LastFmLocationPtr)
Q_DECLARE_METATYPE(LastFmVenue)
Q_DECLARE_METATYPE(LastFmVenuePtr)

#endif // LASTFMEVENT_H
