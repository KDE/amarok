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

#include "NepomukLabel.h"
#include "NepomukTrack.h"

#include "core/meta/Meta.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include <Nepomuk/Resource>

using namespace Meta;

NepomukTrack::NepomukTrack( const QUrl &resUri, NepomukCollection *coll )
    : Track()
    , m_artist( 0 )
    , m_genre( 0 )
    , m_composer( 0 )
    , m_album( 0 )
    , m_year( 0 )
    , m_labellist()
    , m_coll( coll )
{
    m_resource = Nepomuk::Resource( resUri );
}

NepomukTrack::~NepomukTrack()
{
    // TODO
}

QString
NepomukTrack::name() const
{
    return m_name;
}


QString
NepomukTrack::prettyName() const
{
    return m_name;
}

KUrl
NepomukTrack::playableUrl() const
{
    return m_playableUrl;
}

QString
NepomukTrack::prettyUrl() const
{
    // check if path() or prettyUrl() should be used
    return m_playableUrl.prettyUrl();
}

QString
NepomukTrack::uidUrl() const
{
    return m_uidUrl;
}

bool
NepomukTrack::isPlayable() const
{
    return m_playableUrl.isValid();
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
    return m_bpm;
}

QString
NepomukTrack::comment() const
{
    return m_comment;
}

double
NepomukTrack::score() const
{
    // TODO
    return double( 0 );
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

    return m_length;
}

int
NepomukTrack::filesize() const
{
    return m_filesize;
}

int
NepomukTrack::sampleRate() const
{
    return m_sampleRate;
}

int
NepomukTrack::bitrate() const
{
    return m_bitrate;
}

QDateTime
NepomukTrack::createDate() const
{
    return m_createDate;
}

QDateTime
NepomukTrack::modifyDate() const
{
    return m_modifyDate;
}

int
NepomukTrack::trackNumber() const
{
    return m_trackNumber;
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
    return m_type;
}

void
NepomukTrack::setArtist( NepomukArtistPtr artist )
{
    m_artist = Meta::ArtistPtr::staticCast( artist );
}

void
NepomukTrack::setComposer( NepomukComposerPtr composer )
{
    m_composer = Meta::ComposerPtr::staticCast( composer );
}

void
NepomukTrack::setGenre( NepomukGenrePtr genre )
{
    m_genre = Meta::GenrePtr::staticCast( genre );
}

void
NepomukTrack::setAlbum( NepomukAlbumPtr album )
{
    m_album = Meta::AlbumPtr::staticCast( album );
}

void
NepomukTrack::setYear( NepomukYearPtr year )
{
    m_year = Meta::YearPtr::staticCast( year );
}

void
NepomukTrack::setName( const QString &name )
{
    m_name = name;
}

void
NepomukTrack::setType( const QString &type )
{
    m_type = type;
}

void
NepomukTrack::setLength( const qint64 length )
{
    m_length = length;
}

void
NepomukTrack::setBitrate( const int rate )
{
    m_bitrate = rate;
}

void
NepomukTrack::setTrackNumber( const int trackNumber )
{
    m_trackNumber = trackNumber;
}

void
NepomukTrack::setUidUrl( const QString &uidUrl )
{
    m_uidUrl = uidUrl;
}

void
NepomukTrack::setDiscNumber( const int discNumber )
{
    m_discNumber = discNumber;
}

void
NepomukTrack::setModifyDate( const QDateTime &modifyDate )
{
    m_modifyDate = modifyDate;
}

void
NepomukTrack::setCreateDate( const QDateTime &createDate )
{
    m_createDate = createDate;
}

void
NepomukTrack::setbpm( const qreal bpm )
{
    m_bpm = bpm;
}

void
NepomukTrack::setComment( const QString &comment )
{
    m_comment = comment;
}

void
NepomukTrack::setSampleRate( const int sampleRate )
{
    m_sampleRate = sampleRate;
}

void
NepomukTrack::setFilesize( const int filesize )
{
    m_filesize = filesize;
}

void
NepomukTrack::setTrackGain( const double trackGain )
{
    m_trackGain = trackGain;
}

void NepomukTrack::setTrackPeakGain( const double trackPeakGain )
{
    m_trackPeakGain = trackPeakGain;
}

void
NepomukTrack::setAlbumGain( const double albumGain )
{
    m_albumGain = albumGain;
}

void NepomukTrack::setAlbumPeakGain( const double albumPeakGain )
{
    m_albumPeakGain = albumPeakGain;
}

void
NepomukTrack::setPlayableUrl( const KUrl &url )
{
    m_playableUrl = url;
}

// non pure virtual functions
bool
NepomukTrack::inCollection() const
{
    if( m_resource.isValid() )
        return true;
    else return false;
}

Collections::Collection*
NepomukTrack::collection() const
{
    return m_coll;
}

qreal
NepomukTrack::replayGain( ReplayGainTag mode ) const
{
    qreal gain = 0;
    switch( mode )
    {
    case 0 :
        gain = m_trackGain;
        break;
    case 1 :
        gain = m_trackPeakGain;
        break;
    case 2 :
        gain = m_albumGain;
        break;
    case 3 :
        gain = m_albumPeakGain;
        break;
    }

    // TODO
    // Should return the track replay gain if the album
    // gain is requested but is not available.

    return gain;
}

void
NepomukTrack::addLabel( const Meta::LabelPtr &label )
{
    m_labellist.append( label );
}

void
NepomukTrack::addLabel( const QString &label )
{
    NepomukLabelPtr labelPtr;
    labelPtr = new NepomukLabel( label );
    m_labellist.append( Meta::LabelPtr::staticCast( labelPtr ) );
}

Meta::LabelList
NepomukTrack::labels() const
{
    return m_labellist;
}

void
NepomukTrack::removeLabel( const LabelPtr &label )
{
    m_labellist.removeOne( label );
}
