#include "AudioCdAlbum.h"
#include "AudioCdTrack.h"

#include "core/support/Debug.h"
#include "covermanager/CoverCache.h"

using namespace Meta;

AudioCdAlbum::AudioCdAlbum( const QString &name, const QString &albumArtist )
    : Meta::Album()
    , m_name( name )
    , m_albumArtist( albumArtist )
    , m_isCompilation( false )
{
}

AudioCdAlbum::~AudioCdAlbum()
{
    CoverCache::invalidateAlbum( this );
}

QString
AudioCdAlbum::name() const
{
    return m_name;
}

bool
AudioCdAlbum::isCompilation() const
{
    return m_isCompilation;
}

bool
AudioCdAlbum::canUpdateCompilation() const
{
    return true;
}

void
AudioCdAlbum::setCompilation( bool compilation )
{
    m_isCompilation = compilation;
}

bool
AudioCdAlbum::hasAlbumArtist() const
{
    return !m_albumArtist.isNull();
}

ArtistPtr
AudioCdAlbum::albumArtist() const
{
    return Meta::ArtistPtr( new AudioCdArtist( m_albumArtist ) );
}

TrackList
AudioCdAlbum::tracks()
{
    return Meta::TrackList();
}

QImage
AudioCdAlbum::image( int size ) const
{
    return Meta::Album::image( size );
}

bool
AudioCdAlbum::hasImage( int size ) const
{
    return Meta::Album::hasImage( size );
}

bool
AudioCdAlbum::canUpdateImage() const
{
    return false;
}

void
AudioCdAlbum::setAlbumArtist( QString& artist )
{
    m_albumArtist = artist;
}