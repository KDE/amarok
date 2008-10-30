/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd.                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *    This program is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef LASTFM_TRACK_H
#define LASTFM_TRACK_H

#include <lastfm/DllExportMacro.h>
#include <lastfm/types/Album.h>
#include <lastfm/types/Artist.h>
#include <lastfm/core/WeightedStringList.h>
#include <QDateTime>
#include <QDomElement>
#include <QExplicitlySharedDataPointer>
#include <QString>
#include <QMap>
#include <QUrl>


struct TrackData : QSharedData
{
    TrackData();

    QString artist;
    QString album;
    QString title;
    uint trackNumber;
    uint duration;
    short source;
    short rating;
    QString mbid; /// musicbrainz id
    QString fingerprintId;
    QUrl url;
    QDateTime time; /// the time the track was started at

    //FIXME I hate this, but is used for radio trackauth etc.
    QMap<QString,QString> extras;
};


class LASTFM_TYPES_DLLEXPORT Track
{
public:
    enum Source
    {
        // DO NOT UNDER ANY CIRCUMSTANCES CHANGE THE ORDER OR VALUES OF THIS ENUM!
        // you will cause broken settings and b0rked scrobbler cache submissions

        Unknown = -1,
        LastFmRadio,
        Player,
        MediaDevice,
		NonPersonalisedBroadcast, // eg Shoutcast, BBC Radio 1, etc.
		PersonalisedRecommendation, // eg Pandora, but not Last.fm
    };

    Track();
    explicit Track( const QDomElement& );

    /** this track and that track point to the same object, so they are the same
      * in fact. This doesn't do a deep data comparison. So even if all the 
      * fields are the same it will return false if they aren't in fact spawned
      * from the same initial Track object */
    bool operator==( const Track& that ) const
    {
        return this->d == that.d;
    }
    bool operator!=( const Track& that ) const
    {
        return !operator==( that );
    }

    /** not a great isEmpty check, but most services will complain if these two
      * are empty */
    bool isNull() const { return d->artist.isEmpty() && d->title.isEmpty(); }


    Artist artist() const { return Artist( d->artist ); }
    Album album() const { return Album( artist(), d->album ); }
    QString title() const { return d->title; }
    uint trackNumber() const { return d->trackNumber; }
    uint duration() const { return d->duration; }
    QString mbid() const { return d->mbid; }
    QUrl url() const { return d->url; }
    QDateTime timestamp() const { return d->time; }
    Source source() const { return (Source)d->source; }
    QString fingerprintId() const { return d->fingerprintId; }

    QString durationString() const;

    /** default separator is an en-dash */
    QString toString( const QChar& separator = QChar(8211) ) const;
    /** the standard representation of this object as an XML node */
    QDomElement toDomElement( class QDomDocument& ) const;

    
//////////// TODO move to class Scrobble
    bool operator<( const Track &that ) const
    {
        return this->d->time < that.d->time;
    }
    
//////////// lastfm::Ws
    
	/** See last.fm/api Track section */
    WsReply* share( const class User& recipient, const QString& message = "" );

    /** you can get any WsReply TagList using Tag::list( WsReply* ) */
	WsReply* getTags() const; // for the logged in user
	WsReply* getTopTags() const;
    
    /** you can only add 10 tags, we submit everything you give us, but the
      * docs state 10 only */
    WsReply* addTags( const QStringList& ) const;

	/** the url for this track's page at last.fm */
	QUrl www() const;
	
protected:
    QExplicitlySharedDataPointer<TrackData> d;
};


class LASTFM_TYPES_DLLEXPORT MutableTrack : public Track
{
public:
    MutableTrack()
    {}

    MutableTrack( const Track& that ) : Track( that )
    {}

    void setArtist( QString artist ) { d->artist = artist.trimmed(); }
    void setAlbum( QString album ) { d->album = album.trimmed(); }
    void setTitle( QString title ) { d->title = title.trimmed(); }
    void setTrackNumber( uint n ) { d->trackNumber = n; }
    void setDuration( uint duration ) { d->duration = duration; }
    void setUrl( QUrl url ) { d->url = url; }
    void setSource( Source s ) { d->source = s; }
    
    void setMbid( Mbid id ) { d->mbid = id; }
    void setFingerprintId( QString id ) { d->fingerprintId = id; }
    
    WsReply* love();
    WsReply* ban();

    /** currently doesn't work, as there is no webservice */
    void unlove();
	
    void stamp() { d->time = QDateTime::currentDateTime(); }

    void setExtra( QString key, QString value ) { d->extras[key] = value; }
};


inline 
TrackData::TrackData() 
             : trackNumber( 0 ),
               duration( 0 ),
               source( Track::Unknown ),
               rating( 0 )
{}


#include <QDebug>
inline QDebug operator<<( QDebug d, const Track& t )
{
    return d << t.toString( '-' ) << t.album();
}


#include <QMetaType>
Q_DECLARE_METATYPE( Track );

#endif
