/****************************************************************************************
 * Copyright (c) 2012 Phalgun Guduthur <me@phalgun.in>                                  *
 * Copyright (c) 2013 Edward Toroshchin <amarok@hades.name>                             *
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

#include "NepomukCollection.h"
#include "NepomukLabel.h"

#include "core/meta/Meta.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include <KLocalizedString>
#include <Nepomuk2/Resource>
#include <Nepomuk2/Tag>
#include <Nepomuk2/Variant>
#include <Nepomuk2/Vocabulary/NUAO>
#include <Soprano/Vocabulary/NAO>

using namespace Meta;

NepomukTrack::NepomukTrack( const QUrl &resUri, Collections::NepomukCollection *coll )
    : m_filled( false )
    , m_length( 0 )
    , m_bitrate( 0 )
    , m_trackNumber( 0 )
    , m_discNumber( 0 )
    , m_bpm( 0.0 )
    , m_sampleRate( 0 )
    , m_filesize( 0 )
    , m_trackGain( 0.0 )
    , m_trackPeakGain( 0.0 )
    , m_albumGain( 0.0 )
    , m_albumPeakGain( 0.0 )
    , m_coll( coll )
    , m_resourceUri( resUri )
{
}

NepomukTrack::~NepomukTrack()
{
}

QString
NepomukTrack::name() const
{
    return m_name;
}

QUrl
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
    return m_resourceUri.toString();
}

QString
NepomukTrack::notPlayableReason() const
{
    if( !m_playableUrl.isValid() )
        return i18n( "Invalid URL" );
    return QString();
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
    return m_discNumber;
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

QString
NepomukTrack::type() const
{
    return m_type;
}

bool
NepomukTrack::inCollection() const
{
    return m_coll;
}

Collections::Collection*
NepomukTrack::collection() const
{
    return m_coll;
}

void
NepomukTrack::addLabel( const Meta::LabelPtr &label )
{
    if( !label )
        return;

    const NepomukLabel *nlabel = dynamic_cast<const NepomukLabel*>(label.data());
    if( nlabel )
    {
        resource()->addTag( nlabel->tag() );
        notifyObservers();
    }
    else
    {
        addLabel( label->name() );
    }
}

void
NepomukTrack::addLabel( const QString &label )
{
    Nepomuk2::Tag tag;
    tag.setLabel( label );
    resource()->addTag( tag );

    notifyObservers();
}

Meta::LabelList
NepomukTrack::labels() const
{
    LabelList result;

    foreach( const Nepomuk2::Tag &tag, resource()->tags() )
        result << NepomukLabel::fromNepomukTag( m_coll, tag );

    return result;
}

void
NepomukTrack::removeLabel( const LabelPtr &label )
{
    const NepomukLabel *nlabel = dynamic_cast<const NepomukLabel*>(label.data());

    if( !nlabel ) return;

    resource()->removeProperty( Soprano::Vocabulary::NAO::hasTag(), nlabel->tag() );

    notifyObservers();
}

StatisticsPtr
NepomukTrack::statistics()
{
    return StatisticsPtr( this );
}

int
NepomukTrack::rating() const
{
    return resource()->rating();
}

void
NepomukTrack::setRating( int newRating )
{
    resource()->setRating( newRating );
    notifyObservers();
}

QDateTime
NepomukTrack::lastPlayed() const
{
    return resource()->property( Nepomuk2::Vocabulary::NUAO::lastUsage() ).toDateTime();
}

void
NepomukTrack::setLastPlayed( const QDateTime &date )
{
    resource()->setProperty( Nepomuk2::Vocabulary::NUAO::lastUsage(), date );
    notifyObservers();
}

QDateTime
NepomukTrack::firstPlayed() const
{
    return resource()->property( Nepomuk2::Vocabulary::NUAO::firstUsage() ).toDateTime();
}

void
NepomukTrack::setFirstPlayed( const QDateTime &date )
{
    resource()->setProperty( Nepomuk2::Vocabulary::NUAO::firstUsage(), date );
    notifyObservers();
}

int
NepomukTrack::playCount() const
{
    return resource()->usageCount();
}

void
NepomukTrack::setPlayCount( int newPlayCount )
{
    resource()->setProperty( Nepomuk2::Vocabulary::NUAO::usageCount(), newPlayCount );
    notifyObservers();
}

Nepomuk2::Resource*
NepomukTrack::resource() const
{
    if( !m_resource )
        m_resource.reset(new Nepomuk2::Resource( m_resourceUri ));
    return m_resource.data();
}
