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

    m_playableUrl = NULL;
    m_name = "";
    m_type = "";
    m_length = 0;
    m_bitrate = 0;
    m_trackNumber = 0;
    m_uidUrl = "";
    m_discNumber = 0;
    m_bpm = 0;
    m_comment = "";
    m_sampleRate = 0;
    m_filesize = 0;
    m_trackGain = 0;
    m_trackPeakGain = 0;
    m_albumGain = 0;
    m_albumPeakGain = 0;
}

NepomukTrack::~NepomukTrack()
{
}

QString
NepomukTrack::name() const
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
    return m_playableUrl.path();
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
    return 0;
}

void
NepomukTrack::setScore( double newScore )
{
    // TODO
    // Nepomuk is yet to have a concept of scores
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
    //TODO No disc number equivalent ontology yet
    return 0;
}

int
NepomukTrack::playCount() const
{
    return m_resource.usageCount();
}

QString
NepomukTrack::type() const
{
    return m_type;
}

void
NepomukTrack::setArtist( ArtistPtr artist )
{
    m_artist = artist;
}

void
NepomukTrack::setComposer( ComposerPtr composer )
{
    m_composer = composer;
}

void
NepomukTrack::setGenre( GenrePtr genre )
{
    m_genre = genre;
}

void
NepomukTrack::setAlbum( AlbumPtr album )
{
    m_album = album;
}

void
NepomukTrack::setYear( YearPtr year )
{
    m_year = year;
}

void
NepomukTrack::addLabel( const Meta::LabelPtr &label )
{
    if( !m_labellist.contains( label ) )
        m_labellist.append( label );
}

void
NepomukTrack::addLabel( const QString &label )
{
    LabelPtr labelPtr;
    labelPtr = new NepomukLabel( label );
    m_labellist.append( labelPtr );
}

Meta::LabelList
NepomukTrack::labels() const
{
    return m_labellist;
}

void
NepomukTrack::removeLabel( const LabelPtr &label )
{
    m_labellist.removeAll( label );
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
NepomukTrack::setBitrate( int rate )
{
    m_bitrate = rate;
}

void
NepomukTrack::setTrackNumber( int trackNumber )
{
    m_trackNumber = trackNumber;
}

void
NepomukTrack::setUidUrl( const QString &uidUrl )
{
    m_uidUrl = uidUrl;
}

void
NepomukTrack::setDiscNumber( int discNumber )
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
NepomukTrack::setSampleRate( int sampleRate )
{
    m_sampleRate = sampleRate;
}

void
NepomukTrack::setFilesize( int filesize )
{
    m_filesize = filesize;
}

void
NepomukTrack::setTrackGain( qreal trackGain )
{
    m_trackGain = trackGain;
}

void NepomukTrack::setTrackPeakGain( qreal trackPeakGain )
{
    m_trackPeakGain = trackPeakGain;
}

void
NepomukTrack::setAlbumGain( qreal albumGain )
{
    m_albumGain = albumGain;
}

void NepomukTrack::setAlbumPeakGain( qreal albumPeakGain )
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
        case ReplayGain_Track_Gain :
            gain = m_trackGain;
            break;
        case ReplayGain_Track_Peak :
            gain = m_trackPeakGain;
            break;
        case ReplayGain_Album_Gain :
            gain = m_albumGain;
            if( gain == 0 )
                gain = m_trackGain;
            break;
        case ReplayGain_Album_Peak :
            gain = m_albumPeakGain;
            if( gain == 0 )
                gain = m_trackPeakGain;
            break;
    }

    return gain;
}
