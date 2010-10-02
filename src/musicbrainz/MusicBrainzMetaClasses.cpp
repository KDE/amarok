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

#include "MusicBrainzMetaClasses.h"


const QStringList mb_artist_types( QStringList()
                                << "Group"
                                << "Person"
);
const QStringList mb_release_types( QStringList()
                                << "None"
                                << "Album"
                                << "Single"
                                << "EP"
                                << "Compilation"
                                << "Soundtrack"
                                << "Spokenword"
                                << "Interview"
                                << "Audiobook"
                                << "Live"
                                << "Remix"
                                << "Other"
                                << "Official"
                                << "Promotion"
                                << "Bootleg"
                                << "Pseudo-Release"
);

MusicBrainzArtist::MusicBrainzArtist( const QString &Id )
                 : m_artistId( Id )
{

}

MusicBrainzArtist::MusicBrainzArtist( const MusicBrainzArtist &artist )
                 : m_artistId( artist.id() )
{
    m_dateBegin = artist.dateBegin();
    m_dateEnd = artist.dateEnd();
    m_description = artist.description();
    m_name = artist.name();
    m_sortName = artist.sortName();
    m_type = artist.type();
}


QString
MusicBrainzArtist::id() const
{
    return m_artistId;
}

void
MusicBrainzArtist::setId(const QString &Id)
{
    m_artistId = Id;
}

QString
MusicBrainzArtist::name() const
{
    return m_name;
}

void
MusicBrainzArtist::setName( const QString &name )
{
    m_name = name;
}

QString
MusicBrainzArtist::sortName() const
{
    return m_sortName;
}

void
MusicBrainzArtist::setSortName( const QString &name )
{
    m_sortName = name;
}

QString
MusicBrainzArtist::description() const
{
    return m_description;
}

void
MusicBrainzArtist::setDescription( const QString &description )
{
    m_description = description;
}

QDate
MusicBrainzArtist::dateBegin() const
{
    return m_dateBegin;
}

QDate
MusicBrainzArtist::dateEnd() const
{
    return m_dateEnd;
}

void
MusicBrainzArtist::setDateBegin( const QString &date )
{
    if( date.length() < 4 )
        return;
    else if( date.length() < 5 )
        m_dateBegin = QDate::fromString( date, "YYYY" );
    else if( date.length() < 8 )
        m_dateBegin = QDate::fromString( date, "YYYY-MM" );
    else
        m_dateBegin = QDate::fromString( date, Qt::ISODate );
}

void
MusicBrainzArtist::setDateEnd( const QString &date )
{
    if( date.length() < 4 )
        return;
    else if( date.length() < 7 )
        m_dateEnd = QDate::fromString( date, "YYYY" );
    else if( date.length() < 10 )
        m_dateEnd = QDate::fromString( date, "YYYY-MM" );
    else
        m_dateEnd = QDate::fromString( date, Qt::ISODate );
}

QString
MusicBrainzArtist::type() const
{
    return m_type;
}

void
MusicBrainzArtist::setType( const QString &type )
{
    if( mb_artist_types.contains( type ) )
        m_type = type;
}

MusicBrainzRelease::MusicBrainzRelease( const QString &Id )
                  : m_releaseId( Id )
{

}

MusicBrainzRelease::MusicBrainzRelease( const MusicBrainzRelease &releas )
                  : m_releaseId( releas.id() )
{
    m_title = releas.title();
    m_artistId = releas.artistId();
    m_length = releas.length();
    m_language = releas.language();
    m_trackIds.append( releas.tracks() );
    m_type = releas.type();
    m_year = releas.year();
}

QString
MusicBrainzRelease::id() const
{
    return m_releaseId;
}

void MusicBrainzRelease::setId(const QString &Id)
{
    m_releaseId = Id;
}

QString
MusicBrainzRelease::title() const
{
    return m_title;
}

void
MusicBrainzRelease::setTitle( const QString &title )
{
    m_title = title;
}

QString
MusicBrainzRelease::artistId() const
{
    return m_artistId;
}

void
MusicBrainzRelease::setArtistId( const QString &artistId )
{
    m_artistId = artistId;
}

int
MusicBrainzRelease::length() const
{
    return m_length;
}

void
MusicBrainzRelease::setLength( const int length )
{
    m_length = length;
}


QString
MusicBrainzRelease::language() const
{
    return m_language;
}

void
MusicBrainzRelease::setLanguage( const QString &language )
{
    m_language = language;
}

QString MusicBrainzRelease::type() const
{
    return m_type;
}

void
MusicBrainzRelease::setType( const QString &type )
{
    QStringList _types;
    foreach( QString str, type.split( " ", QString::SkipEmptyParts ) )
    {
        if( mb_release_types.contains( str, Qt::CaseInsensitive ) )
            _types << str;
    }
    if( !_types.isEmpty() )
        m_type = _types.join( " " );
}

int
MusicBrainzRelease::year() const
{
    return m_year;
}

void
MusicBrainzRelease::setYear( const int year )
{
    m_year = year;
}

QStringList
MusicBrainzRelease::tracks() const
{
    return m_trackIds;
}

int
MusicBrainzRelease::tracksCount() const
{
    return m_trackIds.count();
}

QString
MusicBrainzRelease::track( const int n )
{
    return ( n >= m_trackIds.count() )?QString():m_trackIds[ n ];
}

void
MusicBrainzRelease::addTrack( const QString &trackId )
{
    m_trackIds << trackId;
}

MusicBrainzTrack::MusicBrainzTrack( const QString &Id )
                : m_trackId( Id )
{

}

MusicBrainzTrack::MusicBrainzTrack( const MusicBrainzTrack &track )
                : m_trackId( track.id() )
{
    m_artistId = track.artistId();
    m_length = track.length();
    m_title = track.title();
    m_track = track.track();
    m_releaseOffsets.append( track.releaseOffsets() );
    m_releases.append( track.releases() );
}

QString
MusicBrainzTrack::id() const
{
    return m_trackId;
}

void MusicBrainzTrack::setId(const QString &Id)
{
    m_trackId = Id;
}

QString
MusicBrainzTrack::title() const
{
    return m_title;
}

void
MusicBrainzTrack::setTitle(const QString &title)
{
    m_title = title;
}

int
MusicBrainzTrack::length() const
{
    return m_length;
}

void
MusicBrainzTrack::setLength(int length)
{
    m_length = length;
}

QString
MusicBrainzTrack::artistId() const
{
    return m_artistId;
}

void
MusicBrainzTrack::setArtistId(const QString &artistId)
{
    m_artistId = artistId;
}

Meta::TrackPtr
MusicBrainzTrack::track() const
{
    return m_track;
}

void
MusicBrainzTrack::setTrack(Meta::TrackPtr track)
{
    m_track = track;
}

QStringList
MusicBrainzTrack::releases() const
{
    return m_releases;
}

QList < int >
MusicBrainzTrack::releaseOffsets() const
{
    return m_releaseOffsets;
}

int
MusicBrainzTrack::releasesCount() const
{
    return m_releases.count();
}

QString
MusicBrainzTrack::releaseId(const int n) const
{
    return ( n >= m_releases.count() )?QString():m_releases[ n ];
}

int
MusicBrainzTrack::releaseOffset(const int n) const
{
    return ( n >= m_releaseOffsets.count() )?0:m_releaseOffsets[ n ];
}

void
MusicBrainzTrack::addRelease(const QString &releaseId, const int offset)
{
    m_releases << releaseId;
    m_releaseOffsets << offset;
}
