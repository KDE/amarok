/****************************************************************************************
 * Copyright (c) 2010 Nikhil Marathe <nsm.nikhil@gmail.com>                             *
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

#include "UpnpMeta.h"

#include "UpnpCollectionBase.h"
#include "core/support/Debug.h"
#include "covermanager/CoverCache.h"
#include "covermanager/CoverFetchingActions.h"
#include "core-impl/capabilities/AlbumActionsCapability.h"

#include <QAction>

#include <KIO/CopyJob>

using namespace Meta;

UpnpTrack::UpnpTrack( Collections::UpnpCollectionBase *collection )
    : Meta::Track()
    , m_collection( collection )
    , m_album( 0 )
    , m_genre( 0 )
    , m_composer( 0 )
    , m_year( 0 )
{
}

UpnpTrack::~UpnpTrack()
{
    //nothing to do
}

QString
UpnpTrack::name() const
{
    return m_name;
}

QUrl
UpnpTrack::playableUrl() const
{

    QUrl url( m_playableUrl );
    return url;
}

QString
UpnpTrack::uidUrl() const
{
    return m_uidUrl;
}

QString
UpnpTrack::prettyUrl() const
{
    return m_playableUrl;
}

QString
UpnpTrack::notPlayableReason() const
{
    return networkNotPlayableReason();
}

AlbumPtr
UpnpTrack::album() const
{
    return AlbumPtr::staticCast( m_album );
}

ArtistPtr
UpnpTrack::artist() const
{
    return ArtistPtr::staticCast( m_artist );
}

GenrePtr
UpnpTrack::genre() const
{
    return GenrePtr::staticCast( m_genre );
}

ComposerPtr
UpnpTrack::composer() const
{
    return ComposerPtr::staticCast( m_composer );
}

YearPtr
UpnpTrack::year() const
{
    return YearPtr::staticCast( m_year );
}

void
UpnpTrack::setAlbum( const QString &newAlbum )
{
    Q_UNUSED( newAlbum )
}

void
UpnpTrack::setArtist( const QString &newArtist )
{
    Q_UNUSED( newArtist )
}

void
UpnpTrack::setComposer( const QString &newComposer )
{
    Q_UNUSED( newComposer )
}

void
UpnpTrack::setGenre( const QString &newGenre )
{
    Q_UNUSED( newGenre )
}

void
UpnpTrack::setYear( int newYear )
{
    Q_UNUSED( newYear )
}

void
UpnpTrack::setUidUrl( const QString &url )
{
// TODO should we include uuid() also in the url?
    m_uidUrl = url;
    if( !url.startsWith( "upnp-ms://" ) )
        m_uidUrl = "upnp-ms://" + m_collection->collectionId() + "/" + m_uidUrl;
}

void
UpnpTrack::setPlayableUrl( const QString& url )
{
    m_playableUrl = url;
}

/* 
TODO: This isn't good enough, but for now as daapreader/Reader.cpp indicates
 we can query for the BPM from daap server, but desire is to get BPM of files working
 first! 
*/
qreal
UpnpTrack::bpm() const
{
    return -1.0;
}

QString
UpnpTrack::comment() const
{
    return QString();
}

void
UpnpTrack::setComment( const QString &newComment )
{
    Q_UNUSED( newComment )
}

qint64
UpnpTrack::length() const
{
    return m_length;
}

int
UpnpTrack::filesize() const
{
    return 0;
}

int
UpnpTrack::sampleRate() const
{
    return 0;
}

int
UpnpTrack::bitrate() const
{
    return m_bitrate;
}

int
UpnpTrack::trackNumber() const
{
    return m_trackNumber;
}

void
UpnpTrack::setTrackNumber( int newTrackNumber )
{
    m_trackNumber = newTrackNumber;
}

int
UpnpTrack::discNumber() const
{
    return 0;
}

void
UpnpTrack::setDiscNumber( int newDiscNumber )
{
    Q_UNUSED( newDiscNumber )
}

QString
UpnpTrack::type() const
{
    return m_type;
}

bool
UpnpTrack::inCollection() const
{
    return true;
}

Collections::Collection*
UpnpTrack::collection() const
{
    return m_collection;
}

void
UpnpTrack::setAlbum( const UpnpAlbumPtr &album )
{
    m_album = album;
}

void
UpnpTrack::setArtist( const UpnpArtistPtr &artist )
{
    m_artist = artist;
}

void
UpnpTrack::setGenre( const UpnpGenrePtr &genre )
{
    m_genre = genre;
}

void
UpnpTrack::setComposer( const UpnpComposerPtr &composer )
{
    m_composer = composer;
}

void
UpnpTrack::setYear( const UpnpYearPtr &year )
{
    m_year = year;
}

void
UpnpTrack::setTitle( const QString &title )
{
    m_name = title;
}

void
UpnpTrack::setLength( qint64 length )
{
    m_length = length;
}

void
UpnpTrack::setBitrate( int rate )
{
    m_bitrate = rate;
}

//UpnpArtist

UpnpArtist::UpnpArtist( const QString &name )
    : Meta::Artist()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

UpnpArtist::~UpnpArtist()
{
    //nothing to do
}

QString
UpnpArtist::name() const
{
    return m_name;
}

TrackList
UpnpArtist::tracks()
{

    return m_tracks;
}

void
UpnpArtist::addTrack( const UpnpTrackPtr &track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
UpnpArtist::removeTrack( const UpnpTrackPtr &track )
{
    m_tracks.removeOne( TrackPtr::staticCast( track ) );
}

UpnpAlbum::UpnpAlbum( const QString &name )
    : QObject()
    , Meta::Album()
    , m_name( name )
    , m_tracks()
    , m_isCompilation( false )
    , m_albumArtist( 0 )
{
    //nothing to do
}

UpnpAlbum::~UpnpAlbum()
{
    CoverCache::invalidateAlbum( this );
}

QString
UpnpAlbum::name() const
{
    return m_name;
}

bool
UpnpAlbum::isCompilation() const
{
    return m_isCompilation;
}

bool
UpnpAlbum::hasAlbumArtist() const
{
    return !m_albumArtist.isNull();
}

ArtistPtr
UpnpAlbum::albumArtist() const
{
    return ArtistPtr::staticCast( m_albumArtist );
}

TrackList
UpnpAlbum::tracks()
{
    return m_tracks;
}

bool
UpnpAlbum::hasImage( int size ) const
{
    Q_UNUSED( size );
    return m_albumArtUrl.isValid();
}

QImage
UpnpAlbum::image( int size ) const
{
    if( m_image.isNull() )
    {
        QString path;
        if( m_albumArtUrl.isValid()
            && KIO::copy( m_albumArtUrl, QUrl::fromLocalFile( path ) )->exec() )
        {
            m_image = QImage( path );
            CoverCache::invalidateAlbum( this );
        }
    }

    if( m_image.isNull() )
        return Meta::Album::image( size );

    return size <= 1 ? m_image : m_image.scaled( size, size );
}

QUrl
UpnpAlbum::imageLocation( int size )
{
    Q_UNUSED( size );
    return m_albumArtUrl;
}

void
UpnpAlbum::addTrack( const UpnpTrackPtr &track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
UpnpAlbum::removeTrack( const UpnpTrackPtr &track )
{
    m_tracks.removeOne( TrackPtr::staticCast( track ) );
}

void
UpnpAlbum::setAlbumArtist( const UpnpArtistPtr &artist )
{
    m_albumArtist = artist;
}

void
UpnpAlbum::setAlbumArtUrl( const QUrl &url )
{
    m_albumArtUrl = url;
}

Capabilities::Capability*
UpnpAlbum::createCapabilityInterface( Capabilities::Capability::Type type )
{
    switch( type )
    {
        case Capabilities::Capability::Actions:
            return new Capabilities::AlbumActionsCapability( Meta::AlbumPtr( this ) );
        default:
            return 0;
    }
}
//UpnpGenre

UpnpGenre::UpnpGenre( const QString &name )
    : Meta::Genre()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

UpnpGenre::~UpnpGenre()
{
    //nothing to do
}

QString
UpnpGenre::name() const
{
    return m_name;
}

TrackList
UpnpGenre::tracks()
{
    return m_tracks;
}

void
UpnpGenre::addTrack( const UpnpTrackPtr &track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
UpnpGenre::removeTrack( const UpnpTrackPtr &track )
{
    m_tracks.removeOne( TrackPtr::staticCast( track ) );
}

//UpnpComposer

UpnpComposer::UpnpComposer( const QString &name )
    : Meta::Composer()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

UpnpComposer::~UpnpComposer()
{
    //nothing to do
}

QString
UpnpComposer::name() const
{
    return m_name;
}

TrackList
UpnpComposer::tracks()
{
    return m_tracks;
}

void
UpnpComposer::addTrack( const UpnpTrackPtr &track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
UpnpComposer::removeTrack( const UpnpTrackPtr &track )
{
    m_tracks.removeOne( TrackPtr::staticCast( track ) );
}

//UpnpYear

UpnpYear::UpnpYear( int name )
    : Meta::Year()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

UpnpYear::~UpnpYear()
{
    //nothing to do
}

QString
UpnpYear::name() const
{
    return m_name;
}

TrackList
UpnpYear::tracks()
{
    return m_tracks;
}

void
UpnpYear::addTrack( const UpnpTrackPtr &track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
UpnpYear::removeTrack( const UpnpTrackPtr &track )
{
    m_tracks.removeOne( TrackPtr::staticCast( track ) );
}

