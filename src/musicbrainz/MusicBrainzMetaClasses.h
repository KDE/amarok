/****************************************************************************************
 * Copyright (c) 2010 Sergey Ivanov <123kash@gmail.com>                                 *
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

#ifndef MUSICBRAINZMETACLASSES_H
#define MUSICBRAINZMETACLASSES_H

#include <QtCore>
#include <core/meta/Meta.h>

namespace Meta
{
    namespace Field
    {
        static const QString UNIQUEIDOWNER       = "id_owner";
    }
}

class MusicBrainzArtist
{
    public:
        MusicBrainzArtist() {};
        MusicBrainzArtist( const QString &Id );
        MusicBrainzArtist( const MusicBrainzArtist &artist );

        QString id() const;
        void setId( const QString &Id );

        QString name() const;
        void setName( const QString &name );

        QString sortName() const;
        void setSortName( const QString &name );

        QString description() const;
        void setDescription( const QString &description );

        QDate dateBegin() const;
        void setDateBegin( const QString &date );

        QDate dateEnd() const;
        void setDateEnd( const QString &date );

        QString type() const;
        void setType( const QString &type );

    private:
        QString m_artistId;
        QString m_name;
        QString m_sortName;
        QString m_description;
        QDate m_dateBegin;
        QDate m_dateEnd;
        QString m_type;
};

class MusicBrainzTrack
{
    public:
        MusicBrainzTrack() {};
        MusicBrainzTrack( const QString &Id );
        MusicBrainzTrack( const MusicBrainzTrack &track );

        QString id() const;
        void setId( const QString &Id );

        QString title() const;
        void setTitle( const QString &title );

        int length() const;
        void setLength( int length);

        QString artistId() const;
        void setArtistId( const QString &artistId );

        Meta::TrackPtr track() const;
        void setTrack(Meta::TrackPtr track);

        QStringList releases() const;
        QList < int > releaseOffsets() const;
        int releasesCount() const;
        void addRelease( const QString &releaseId, const int offset );

        QString releaseId( const int n) const;
        int releaseOffset( const int n ) const;

    private:
        QString m_trackId;
        QString m_title;
        int m_length;
        QString m_artistId;

        QStringList m_releases;
        QList < int > m_releaseOffsets;

        Meta::TrackPtr m_track;
};

class MusicBrainzRelease
{
    public:
        MusicBrainzRelease() {};
        MusicBrainzRelease( const QString &Id );
        MusicBrainzRelease( const MusicBrainzRelease &releas );

        QString id() const;
        void setId( const QString &Id );

        QString title() const;
        void setTitle( const QString &title );

        int length() const;
        void setLength( const int length );

        QString language() const;
        void setLanguage( const QString &language );

        QString artistId() const;
        void setArtistId( const QString &artistId );

        QStringList tracks() const;
        void addTrack( const QString &trackId );
        int tracksCount() const;

        QString track( const int n );

        QString type() const;
        void setType( const QString &type );

        int year() const;
        void setYear( const int year );
    private:
        QString m_releaseId;
        QString m_title;
        int m_length;
        QString m_language;
        QString m_artistId;
        QString m_type;
        int m_year;

        QStringList m_trackIds;
};

typedef QMap< QString, MusicBrainzTrack > MusicBrainzTrackList;
typedef QMap< QString, MusicBrainzArtist > MusicBrainzArtistList;
typedef QMap< QString, MusicBrainzRelease > MusicBrainzReleaseList;


#endif // MUSICBRAINZMETACLASSES_H