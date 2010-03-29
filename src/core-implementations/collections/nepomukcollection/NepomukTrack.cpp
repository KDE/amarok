/****************************************************************************************
 * Copyright (c) 2008 Daniel Winter <dw@danielwinter.de>                                *
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

#include "NepomukAlbum.h"
#include "NepomukArtist.h"
#include "NepomukCollection.h"
#include "NepomukComposer.h"
#include "NepomukGenre.h"
#include "NepomukRegistry.h"
#include "NepomukYear.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core/meta/Meta.h"

#include <QDateTime>
#include <QFile>
#include <QString>
#include <QUrl>

#include <Nepomuk/Resource>
#include <Nepomuk/Variant>

using namespace Meta;

NepomukTrack::NepomukTrack( NepomukCollection *collection, NepomukRegistry *registry, const Soprano::BindingSet &data )
        : Track()
        , m_collection ( collection )
        , m_registry ( registry )
{
    // init all dates to "0" (because amarok defines that date as "never"

    m_firstPlayed = QDateTime::fromTime_t( 0 );
    m_lastPlayed =  QDateTime::fromTime_t( 0 );
    m_createDate = QDateTime::fromTime_t( 0 );

    m_nepores = Nepomuk::Resource( data[ "r"].uri() ) ;
    m_uid = data[ "trackuid" ].toString();
    m_title = data[ collection->getNameForValue( Meta::valTitle ) ].toString();
    m_url = KUrl( data[ collection->getNameForValue( Meta::valUrl ) ].toString() );
    m_artist = data[ collection->getNameForValue( Meta::valArtist ) ].toString();
    m_album = data[ collection->getNameForValue( Meta::valAlbum ) ].toString();
    m_genre = data[ collection->getNameForValue( Meta::valGenre ) ].toString();
    m_type = data[ collection->getNameForValue( Meta::valFormat ) ].toString();
    m_comment = data[ collection->getNameForValue( Meta::valComment ) ].toString();
    m_composer = data[ collection->getNameForValue( Meta::valComposer ) ].toString();
    m_trackNumber = data[ collection->getNameForValue( Meta::valTrackNr ) ]
            .literal().toInt();
    m_length = data[ collection->getNameForValue( Meta::valLength ) ]
            .literal().toInt();
    m_rating = data[ collection->getNameForValue( Meta::valRating ) ]
            .literal().toInt();
    m_bitrate = data[ collection->getNameForValue( Meta::valBitrate ) ]
            .literal().toInt();
    m_discNumber = data[ collection->getNameForValue( Meta::valDiscNr ) ]
            .literal().toInt();
    m_filesize = data[ collection->getNameForValue( Meta::valFilesize ) ]
            .literal().toInt();
    m_playCount = data[ collection->getNameForValue( Meta::valPlaycount ) ]
            .literal().toInt();
    m_sampleRate = data[ collection->getNameForValue( Meta::valSamplerate ) ]
            .literal().toInt();
    m_score = data[ collection->getNameForValue( Meta::valScore ) ]
            .literal().toInt();

    // Soprano gives a warning when they are empty
    Soprano::LiteralValue litval;

    litval = data[ collection->getNameForValue( Meta::valFirstPlayed ) ]
            .literal();
    if ( litval.isDateTime() )
        m_firstPlayed = litval.toDateTime();

    litval = data[ collection->getNameForValue( Meta::valLastPlayed ) ]
            .literal();
    if ( litval.isDateTime() )
        m_lastPlayed = litval.toDateTime();

    litval = data[ collection->getNameForValue( Meta::valCreateDate ) ]
            .literal();
    if ( litval.isDateTime() )
        m_createDate = litval.toDateTime();

    // assuming that Xesam content created is a DateTime, we only want the year
    litval = data[ collection->getNameForValue( Meta::valYear ) ].literal();
    if ( litval.isDateTime() )
        m_year = litval.toDateTime().toString( "yyyy");
}

NepomukTrack::~NepomukTrack()
{
}

QString
NepomukTrack::name() const
{
    return m_title;
}

QString
NepomukTrack::prettyName() const
{
    if (!m_title.isEmpty())
        return m_title;
    else
        return this->m_url.fileName();
}

KUrl
NepomukTrack::playableUrl() const
{
    return m_url;
}

QString
NepomukTrack::uidUrl() const
{
    return "amarokcollnepomukuid://" + m_uid;
}

QString
NepomukTrack::prettyUrl() const
{
    return m_url.path();
}

bool
NepomukTrack::isPlayable() const
{
    //a song is not playable anymore if the collection was removed
    return m_collection && QFile::exists( m_url.path() );
}

bool
NepomukTrack::inCollection() const
{
    return true;
}

Collections::Collection*
NepomukTrack::collection() const
{
    return m_collection;
}

AlbumPtr
NepomukTrack::album() const
{
    return m_collection->registry()->albumForArtistAlbum( m_artist, m_album );
}

ArtistPtr
NepomukTrack::artist() const
{
    return m_collection->registry()->artistForArtistName( m_artist );
}

GenrePtr
NepomukTrack::genre() const
{
    return GenrePtr( new NepomukGenre( m_genre ) );
}

ComposerPtr
NepomukTrack::composer() const
{
    return ComposerPtr( new NepomukComposer( m_composer ) );
}

YearPtr
NepomukTrack::year() const
{
    return YearPtr( new NepomukYear( m_year ) );
}

QString
NepomukTrack::comment() const
{
    return m_comment;
}

double
NepomukTrack::score() const
{
    // TODO: why double? From 0 to 1?
    return double( m_score );
}

void
NepomukTrack::setScore( double newScore )
{
    // scores are between 0 and 1?  Xesam wants them to be int so lets
    // multiply them by 100 (hope that is enough)

    m_lastWrote = QTime::currentTime();

    debug() << "setscore " << endl;
    int tmpScore =  int( newScore*100 );
    m_nepores.setProperty( QUrl( m_collection->getUrlForValue( Meta::valScore ) ), Nepomuk::Variant( tmpScore ) );
    m_score = newScore;
    notifyObservers();

}

int
NepomukTrack::rating() const
{
    return m_rating;
}

void
NepomukTrack::setRating( int newRating )
{
    m_lastWrote = QTime::currentTime();

    m_nepores.setRating( newRating );
    m_rating = newRating;
    notifyObservers();
}

qint64
NepomukTrack::length() const
{
    return m_length * 1000;
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

uint
NepomukTrack::firstPlayed() const
{
    return m_firstPlayed.toTime_t();
}

uint
NepomukTrack::lastPlayed() const
{
    return m_lastPlayed.toTime_t();
}

int
NepomukTrack::playCount() const
{
    return m_playCount;
}

QString
NepomukTrack::type() const
{
    return m_type;
}

void
NepomukTrack::finishedPlaying( double playedFraction )
{
    debug() << "finshedPlaying " << endl;

    // following sql collection only count as play when at least half of the song is played
    // also do not update the other stats (if we assume the track is not played, we should not update the
    // last played date
    if ( playedFraction >= 0.5 )
    {
        m_lastPlayed = QDateTime::currentDateTime();
        if( m_playCount == 0 || m_firstPlayed == QDateTime::fromTime_t( 0 ) )
        {
            m_firstPlayed = m_lastPlayed;
        }
        m_playCount++;
    }

    setScore( Amarok::computeScore( score(), playCount(), playedFraction ) );
    writeStatistics();
    notifyObservers();
}


QString
NepomukTrack::cachedLyrics() const
{
    return m_nepores.property( QUrl( "http://amarok.kde.org/metadata/1.0/track#lyrics" ) ).toString();
}

void
NepomukTrack::setCachedLyrics ( const QString& value )
{
    m_nepores.setProperty( QUrl( "http://amarok.kde.org/metadata/1.0/track#lyrics" ), value );
}

void
NepomukTrack::writeStatistics()
{
    m_lastWrote = QTime::currentTime();
    m_registry->writeToNepomukAsync( m_nepores, QUrl( m_collection->getUrlForValue( Meta::valLastPlayed ) ), Nepomuk::Variant( m_lastPlayed ) );
    m_registry->writeToNepomukAsync( m_nepores, QUrl( m_collection->getUrlForValue( Meta::valPlaycount) ), Nepomuk::Variant( m_playCount ) );
    m_registry->writeToNepomukAsync( m_nepores, QUrl( m_collection->getUrlForValue( Meta::valFirstPlayed) ), Nepomuk::Variant( m_firstPlayed) );
}

QUrl
NepomukTrack::resourceUri() const
{
    return m_nepores.resourceUri();
}

void
NepomukTrack::valueChangedInNepomuk( qint64 value, const Soprano::LiteralValue &literal )
{

    // TODO: find a better way to ignore own writes
    // ignore if last own write is less than 3 seconds ago
    // the change will be our own
    if ( !m_lastWrote.isNull() && m_lastWrote.secsTo( QTime::currentTime() ) < 3 )
        return;

    debug() << "nepo data changed " << m_collection->getNameForValue( value ) << " last wrote " << m_lastWrote.secsTo( QTime::currentTime() ) << endl;
    switch ( value )
    {
        case Meta::valUrl:
            m_url = KUrl ( literal.toString() );
            break;
        case Meta::valRating:
            m_rating = literal.toInt();
            break;
    }
    emit notifyObservers();
}

QString
NepomukTrack::uid() const
{
    return m_uid;
}

void
NepomukTrack::setUid ( const QString& value )
{
    // do not write it to nepomuk, it's nepomukregistrys job
    m_uid = value;
}

Nepomuk::Resource&
NepomukTrack::resource()
{
    return m_nepores;
}

void
NepomukTrack::setResource ( const Nepomuk::Resource& value )
{
    m_nepores = value;
}

