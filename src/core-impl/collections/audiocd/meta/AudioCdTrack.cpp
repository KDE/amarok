#include "AudioCdTrack.h"
#include "AudioCdAlbum.h"
#include "AudioCdCollection.h"

#include "core/support/Debug.h"
#include "covermanager/CoverCache.h"

using namespace Meta;

AudioCdTrack::AudioCdTrack( AudioCdCollection *collection, const KUrl &url )
    : Meta::Track()
    , m_collection( collection )
    , m_album( 0 )
    , m_length( 0 )
    , m_trackNumber( 0 )
    , m_playableUrl( url )
{
}

AudioCdTrack::~AudioCdTrack()
{
    //nothing to do
}

QString
AudioCdTrack::name() const
{
    return m_name;
}

KUrl
AudioCdTrack::playableUrl() const
{
    return m_playableUrl;
}

QString
AudioCdTrack::uidUrl() const
{
    return m_playableUrl.url();
}

QString
AudioCdTrack::prettyUrl() const
{
    return m_playableUrl.prettyUrl();
}

QString
AudioCdTrack::notPlayableReason() const
{
    //TODO: check availablity of correct CD somehow
    return QString();
}

AlbumPtr
AudioCdTrack::album() const
{
    return AlbumPtr::staticCast( m_album );
}

ArtistPtr
AudioCdTrack::artist() const
{
    return Meta::ArtistPtr( new AudioCdArtist( m_artist ) );
}

GenrePtr
AudioCdTrack::genre() const
{
    return Meta::GenrePtr( new AudioCdGenre( m_genre ) );
}

ComposerPtr
AudioCdTrack::composer() const
{
    return Meta::ComposerPtr( new AudioCdComposer( m_composer ) );
}

YearPtr
AudioCdTrack::year() const
{
    return Meta::YearPtr( new AudioCdYear( m_year ) );
}

qreal
AudioCdTrack::bpm() const
{
    return -1.0;
}

QString
AudioCdTrack::comment() const
{
    return QString();
}

void
AudioCdTrack::setComment( const QString &newComment )
{
    Q_UNUSED( newComment )
}

qint64
AudioCdTrack::length() const
{
    return m_length;
}

int
AudioCdTrack::filesize() const
{
    return 0;
}

int
AudioCdTrack::sampleRate() const
{
    return 0;
}

int
AudioCdTrack::bitrate() const
{
    return 0;
}

int
AudioCdTrack::trackNumber() const
{
    return m_trackNumber;
}

void
AudioCdTrack::setTrackNumber( int newTrackNumber )
{
    m_trackNumber = newTrackNumber;
}

int
AudioCdTrack::discNumber() const
{
    return 0;
}

void
AudioCdTrack::setDiscNumber( int newDiscNumber )
{
    Q_UNUSED( newDiscNumber )
}

QString
AudioCdTrack::type() const
{
   return "wav";
}

bool
AudioCdTrack::inCollection() const
{
    return true;
}

Collections::Collection*
AudioCdTrack::collection() const
{
    return m_collection.data();
}

void
AudioCdTrack::setAlbum( AudioCdAlbumPtr album )
{
    m_album = album;
}

void
AudioCdTrack::setArtist( const QString& artist )
{
    m_artist = artist;
}

void
AudioCdTrack::setGenre( const QString& genre )
{
    m_genre = genre;
}

void
AudioCdTrack::setComposer( const QString& composer )
{
    m_composer = composer;
}

void
AudioCdTrack::setYear( const QString& year )
{
    m_year = year;
}

void
AudioCdTrack::setTitle( const QString &title )
{
    m_name = title;
}

void
AudioCdTrack::setLength( qint64 length )
{
    m_length = length;
}