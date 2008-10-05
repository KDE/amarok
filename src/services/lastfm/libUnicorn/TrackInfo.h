/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Christian Muehlhaeuser, Last.fm Ltd <chris@last.fm>                *
 *      Erik Jaelevik, Last.fm Ltd <erik@last.fm>                          *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#ifndef TRACKINFO_H
#define TRACKINFO_H

#include "UnicornDllExportMacro.h"

#include "Track.h"

#ifdef QT_XML_LIB
#include <QDomElement>
#endif

#include <QStringList>
#include <QUrl>


class UNICORN_DLLEXPORT TrackInfo
{
    // undefined because really what consititutes the same track varies depending
    // on usage. Some Qt templates require this operator though so maybe you'll
    // want to add it at some point in the future, my suggestion is don't, it'll
    // cause bugs --mxcl
    bool operator==( const TrackInfo& ) const;

    public:
        enum Source
        {
            //DO NOT UNDER ANY CIRCUMSTANCES CHANGE THE ORDER OF THIS ENUM!
            // you will cause broken settings and b0rked scrobbler cache submissions

            Unknown = -1,
            Radio,
            Player,
            MediaDevice
        };

        enum RatingFlag
        {
            //DO NOT UNDER ANY CIRCUMSTANCES CHANGE THE ORDER OF THIS ENUM!
            // you will cause broken settings and b0rked scrobbler cache submissions

            Skipped = 1,
            Loved = 2,
            Banned = 4,
            Scrobbled = 8
        };

        TrackInfo();
        TrackInfo( const Track& );

        #ifdef QT_XML_LIB
        TrackInfo( const QDomElement& element );
        QDomElement toDomElement( class QDomDocument& document ) const;
        #endif

        void timeStampMe();

        /** compares artist, trackname and album only */
        bool sameAs( const TrackInfo& ) const;

        bool isEmpty() const { return ( m_artist.isEmpty() && m_track.isEmpty() ); }

        const QString artist() const { return m_artist; }
        void setArtist( QString artist ) { m_artist = artist.trimmed(); }

        const QString album() const { return m_album; }
        void setAlbum( QString album ) { m_album = album.trimmed(); }

        const QString track() const { return m_track; }
        void setTrack( QString track ) { m_track = track.trimmed(); }

        const int trackNr() const { return m_trackNr; } //RENAME
        void setTrackNr( int nr ) { m_trackNr = nr; }

        const int playCount() const { return m_playCount; }
        void setPlayCount( int playCount ) { m_playCount = playCount; }

        const int duration() const { return m_duration; }
        QString durationString() const;
        void setDuration( int duration ) { m_duration = duration; }
        void setDuration( QString duration ) { m_duration = duration.toInt(); } //REMOVE

        const QString mbId() const { return m_mbId; }
        void setMbId( QString mbId ) { m_mbId = mbId; }

        const QString path() const;
        void setPath( QString path );

        // A radio track can have more than one path (i.e. URL)
        const QString nextPath() const;
        void setPaths( QStringList paths );
        bool hasMorePaths() { return m_nextPath < m_paths.size(); }

        time_t timeStamp() const { return m_timeStamp; }
        void setTimeStamp( time_t timestamp ) { m_timeStamp = timestamp; }

        const QString fileName() const { return m_fileName; }
        void setFileName( QString fileName ) { m_fileName = fileName; }

        const QString uniqueID() const { return m_uniqueID; }
        void setUniqueID( QString uniqueID ) { m_uniqueID = uniqueID; }

        const Source source() const { return m_source; }
        void setSource( Source s ) { m_source = s; }
        /** scrobbler submission source string code */
        QString sourceString() const;

        /** last.fm authorisation key */
        const QString authCode() const { return m_authCode; } //RENAME
        void setAuthCode( QString code ) { m_authCode = code; }

        void setRatingFlag( RatingFlag flag ) { m_ratingFlags |= flag; }
        bool isSkipped() const { return m_ratingFlags & TrackInfo::Skipped; }
        bool isLoved() const { return m_ratingFlags & TrackInfo::Loved; }
        bool isBanned() const { return m_ratingFlags & TrackInfo::Banned; }
        bool isScrobbled() const { return m_ratingFlags & TrackInfo::Scrobbled; }
        //TODO remove, only used by Scrobbler::scrobble() HACK
        bool isSkippedLovedOrBanned() const { return isSkipped() || isLoved() || isBanned(); }
        void merge( const TrackInfo& that );

        QString toString() const;

        /** only one rating is possible, we have to figure out which from various flags applied */
        QString ratingCharacter() const;

        static TrackInfo fromMimeData( const class QMimeData* mimedata );

        class StopWatch* stopWatch() const { return m_stopWatch; }
        void setStopWatch( StopWatch* watch ) { m_stopWatch = watch; }

        QString playerId() const { return m_playerId; }
        void setPlayerId( QString id ) { m_playerId = id; }

        bool isPowerPlay() const { return !m_powerPlayLabel.isEmpty(); }
        QString powerPlayLabel() const { return m_powerPlayLabel; }
        void setPowerPlayLabel( QString label ) { m_powerPlayLabel = label; }

        QString fpId() const { return m_fpId; }
        void setFpId( QString id ) { m_fpId = id; }

    private:
        QString m_artist;
        QString m_album;
        QString m_track;
        int     m_trackNr; //RENAME

        int     m_playCount;
        int     m_duration;
        QString m_fileName; //RENAME m_path?
        QString m_mbId;
        time_t  m_timeStamp;
        Source  m_source;
        QString m_authCode;
        QString m_uniqueID; //TYPO
        QString m_playerId;
        QString m_powerPlayLabel;

        QStringList m_paths; //WTF
        mutable int m_nextPath; //WTF

        /// Bit of a hack this. Just so that we have an easy way of transporting
        /// the audio controller's timer into the player listener.
        StopWatch* m_stopWatch;  

        short m_ratingFlags;

        /** this cannot be used by any component but the scrobbler, if you rely
          * on it, you will get bugs! */
        //FIXME make reliable?
        QString m_username;

        QString m_fpId;

    public:
        QString username() const { return m_username; }
        void setUsername( const QString& s ) { m_username = s; }
};


#include <QDebug>
inline QDebug operator<<( QDebug& d, const TrackInfo& t )
{
    return d << t.toString();
}


#include <QMetaType>
Q_DECLARE_METATYPE( TrackInfo )

#endif
