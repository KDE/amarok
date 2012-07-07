/****************************************************************************************
 * Copyright (c) 2008 Daniel Winter <dw@danielwinter.de>                                *
 * Copyright (c) 2012 Phalgun Guduthur <me@phalgun.in>                                  *
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

#include "NepomukTrack.h"
#include "NepomukGenre.h"
#include "NepomukComposer.h"
#include "NepomukAlbum.h"
#include "NepomukArtist.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core/meta/Meta.h"

#include <QDateTime>
#include <QFile>
#include <QString>
#include <QUrl>

#include <Nepomuk/Resource>
#include <Nepomuk/File>
#include <Nepomuk/Variant>
#include <Nepomuk/Vocabulary/NIE>
#include <Nepomuk/Vocabulary/NFO>
#include <Nepomuk/Vocabulary/NMM>
#include <Nepomuk/Query/Query>

using namespace Meta;
using namespace Nepomuk::Query;

NepomukTrack::NepomukTrack( Nepomuk::Resource resource )
    : Track()
    , m_resource( resource )

{
    m_kurl = m_resource.toFile().url();
    m_name = m_resource.property( Nepomuk::Vocabulary::NFO::fileName() ).toString();

    QString album = m_resource.property( Nepomuk::Vocabulary::NMM::musicAlbum() ).toString();
    NepomukAlbumPtr albumPtr( new NepomukAlbum( album ) );
    m_album = Meta::AlbumPtr::staticCast( albumPtr );

    QString artist = m_resource.property( Nepomuk::Vocabulary::NMM::performer() ).toString();
    NepomukArtistPtr artistPtr( new NepomukArtist( artist ) );
    m_artist = Meta::ArtistPtr::staticCast( artistPtr );

    QString composer = m_resource.property( Nepomuk::Vocabulary::NMM::composer() ).toString();
    NepomukComposerPtr composerPtr( new NepomukComposer( composer ) );
    m_composer = Meta::ComposerPtr::staticCast( composerPtr );

    QString genre = m_resource.property( Nepomuk::Vocabulary::NMM::genre() ).toString();
    NepomukGenrePtr genrePtr( new NepomukGenre( genre ) );
    m_genre = Meta::GenrePtr::staticCast( genrePtr );

}

NepomukTrack::NepomukTrack( KUrl &fileUrl )
    : Track()
    , m_kurl( fileUrl )
{
    m_resource = Nepomuk::Resource( m_kurl.pathOrUrl() );
    m_name = m_resource.property( Nepomuk::Vocabulary::NFO::fileName() ).toString();

    QString album = m_resource.property( Nepomuk::Vocabulary::NMM::musicAlbum() ).toString();
    NepomukAlbumPtr albumPtr( new NepomukAlbum( album ) );
    m_album = Meta::AlbumPtr::staticCast( albumPtr );


    QString artist = m_resource.property( Nepomuk::Vocabulary::NMM::performer() ).toString();
    NepomukArtistPtr artistPtr( new NepomukArtist( artist ) );
    m_artist = Meta::ArtistPtr::staticCast( artistPtr );

    QString composer = m_resource.property( Nepomuk::Vocabulary::NMM::composer() ).toString();
    NepomukComposerPtr composerPtr( new NepomukComposer( composer ) );
    m_composer = Meta::ComposerPtr::staticCast( composerPtr );

    QString genre = m_resource.property( Nepomuk::Vocabulary::NMM::genre() ).toString();
    NepomukGenrePtr genrePtr( new NepomukGenre( genre ) );
    m_genre = Meta::GenrePtr::staticCast( genrePtr );

}

NepomukTrack::NepomukTrack( ArtistPtr artist,
                            GenrePtr genre,
                            ComposerPtr composer,
                            AlbumPtr album,
                            Nepomuk::Resource resource )
    : m_artist( artist )
    , m_genre( genre )
    , m_composer( composer )
    , m_album( album )
    , m_resource( resource )
{
    m_kurl = m_resource.toFile().url();
    m_name = m_resource.property( Nepomuk::Vocabulary::NFO::fileName() ).toString();
}


QString
NepomukTrack::name() const
{
    //TODO
    // Might have to check for hasProperty in all functions

    return m_name;
}


QString
NepomukTrack::prettyName() const
{
    return m_resource.genericLabel();
}

KUrl
NepomukTrack::playableUrl() const
{
    return m_kurl;
}

QString
NepomukTrack::prettyUrl() const
{
    // check if path() or prettyUrl() should be used
    return m_kurl.prettyUrl();
}

QString
NepomukTrack::uidUrl() const
{
    return m_resource.resourceUri().toString();
}

bool
NepomukTrack::isPlayable() const
{
    if( m_kurl.isLocalFile() )
    {
        return QFile::exists( m_kurl.fileName() ) && m_kurl.isValid();
    }

    else return m_kurl.isValid();
}

AlbumPtr
NepomukTrack::album() const
{
    return m_album;
}

ArtistPtr
NepomukTrack::artist() const
{
    return m_artist;
}

ComposerPtr
NepomukTrack::composer() const
{
    return m_composer;
}

GenrePtr
NepomukTrack::genre() const
{
    return m_genre;
}

YearPtr
NepomukTrack::year() const
{
    return m_year;
}

qreal
NepomukTrack::bpm() const
{
    //check qreal casting
    return qreal( m_resource.property( Nepomuk::Vocabulary::NMM::beatsPerMinute() ).toDouble() );
}

QString
NepomukTrack::comment() const
{
    return m_resource.property( Nepomuk::Vocabulary::NIE::comment() ).toString();
}

double
NepomukTrack::score() const
{
    //TODO
    // ask dr_lepper
    // seriously no clue

    return double( 10 );
}

void
NepomukTrack::setScore( double newScore )
{
    debug() << "Nepomuk Collection, setScore" << newScore;
}

int
NepomukTrack::rating() const
{
    return m_resource.rating();
}

void
NepomukTrack::setRating( int newRating )
{
    m_resource.setRating( newRating );
}

qint64
NepomukTrack::length() const
{
    return m_resource.property( Nepomuk::Vocabulary::NFO::duration() ).toInt64();
}

int
NepomukTrack::filesize() const
{
    // or NFO::fileSize? Ask vHanda
    return m_resource.property( Nepomuk::Vocabulary::NIE::contentSize() ).toInt();
}

int
NepomukTrack::sampleRate() const
{
    return m_resource.property( Nepomuk::Vocabulary::NFO::sampleRate() ).toInt();
}

int
NepomukTrack::bitrate() const
{
    return m_resource.property( Nepomuk::Vocabulary::NFO::averageBitrate() ).toInt();
}

QDateTime
NepomukTrack::createDate() const
{
    return m_resource.property( Nepomuk::Vocabulary::NIE::created() ).toDateTime();
}

QDateTime
NepomukTrack::modifyDate() const
{

    return m_resource.property( Nepomuk::Vocabulary::NIE::contentLastModified() )
           .toDateTime();
}

int
NepomukTrack::trackNumber() const
{
    return m_resource.property( Nepomuk::Vocabulary::NMM::trackNumber() ).toInt();
}

int
NepomukTrack::discNumber() const
{
    //TODO
    return int( 1 );

}

int
NepomukTrack::playCount() const
{
    // ask vHanda
    return m_resource.usageCount();
}

QString
NepomukTrack::type() const
{
    return m_resource.property( Nepomuk::Vocabulary::NFO::codec() ).toString();
}

void
NepomukTrack::notifyObservers() const
{
    //TODO
    // check ipod collection

    //update hash map again
}
