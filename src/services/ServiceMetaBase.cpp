/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "ServiceMetaBase.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "core-impl/meta/default/DefaultMetaTypes.h"

using namespace Meta;

ServiceMetaFactory::ServiceMetaFactory(const QString &dbPrefix)
    : m_dbTablePrefix( dbPrefix )
{
}

QString
ServiceMetaFactory::tablePrefix()
{
    return m_dbTablePrefix;
}

int
ServiceMetaFactory::getTrackSqlRowCount()
{
    return 7;
}

QString
ServiceMetaFactory::getTrackSqlRows()
{
    //subclasses must not change the order of these items, but only append new ones
    return m_dbTablePrefix + "_tracks.id, " +
           m_dbTablePrefix + "_tracks.name, " +
           m_dbTablePrefix + "_tracks.track_number, " +
           m_dbTablePrefix + "_tracks.length, " +
           m_dbTablePrefix + "_tracks.preview_url, " +
           m_dbTablePrefix + "_tracks.album_id, " +
           m_dbTablePrefix + "_tracks.artist_id ";
}

TrackPtr
ServiceMetaFactory::createTrack(const QStringList & rows)
{
    return TrackPtr( new ServiceTrack( rows ) );
}

int
ServiceMetaFactory::getAlbumSqlRowCount()
{
    return 4;
}

QString
ServiceMetaFactory::getAlbumSqlRows()
{
    //subclasses must not change the order of these items, but only append new ones
    return m_dbTablePrefix + "_albums.id, " +
           m_dbTablePrefix + "_albums.name, " +
           m_dbTablePrefix + "_albums.description, " +
           m_dbTablePrefix + "_albums.artist_id ";
}

AlbumPtr
ServiceMetaFactory::createAlbum(const QStringList & rows)
{
    return AlbumPtr( new ServiceAlbum( rows ) );
}

int
ServiceMetaFactory::getArtistSqlRowCount()
{
    return 3;
}

QString
ServiceMetaFactory::getArtistSqlRows()
{
    //subclasses must not change the order of these items, but only append new ones
    return m_dbTablePrefix + "_artists.id, " +
           m_dbTablePrefix + "_artists.name, " +
           m_dbTablePrefix + "_artists.description ";
}

ArtistPtr
ServiceMetaFactory::createArtist(const QStringList & rows)
{
    return ArtistPtr( new ServiceArtist ( rows ) );
}

int
ServiceMetaFactory::getGenreSqlRowCount()
{
    return 2;
}

QString
ServiceMetaFactory::getGenreSqlRows()
{
    //subclasses must not change the order of these items, but only append new ones
    return m_dbTablePrefix + "_genre.id, " +
           m_dbTablePrefix + "_genre.name " ;
}

GenrePtr
ServiceMetaFactory::createGenre(const QStringList & rows)
{
    return GenrePtr( new ServiceGenre ( rows ) );
}

ServiceTrack::ServiceTrack( const QString & name )
    : Meta::Track()
    , ServiceDisplayInfoProvider()
    , CustomActionsProvider()
    , SourceInfoProvider()
    , CurrentTrackActionsProvider()
    , BookmarkThisProvider()
    , m_provider( 0 )
    , m_genre( 0 )
    , m_composer( 0 )
    , m_year( 0 )
    , m_id( 0 )
    , m_trackNumber( 0 )
    , m_length( 0 )
    , m_albumId( 0 )
    , m_albumName( '0' )
    , m_artistId( 0 )
    , m_name( name )
{
}

ServiceTrack::ServiceTrack( const QStringList & resultRow )
    : Meta::Track()
    , ServiceDisplayInfoProvider()
    , CustomActionsProvider()
    , SourceInfoProvider()
    , CurrentTrackActionsProvider()
    , BookmarkThisProvider()
    , m_provider( 0 )
    , m_genre( 0 )
    , m_composer( 0 )
    , m_year( 0 )
{
    m_id = resultRow[0].toInt();
    m_name = resultRow[1];
    m_trackNumber = resultRow[2].toInt();
    m_length = resultRow[3].toInt();
    m_displayUrl = resultRow[4];
    m_playableUrl = resultRow[4];
    m_downloadableUrl = resultRow[4];
    m_albumId = resultRow[5].toInt();
    //m_albumName = resultRow[6];
    m_artistId = resultRow[6].toInt();
    //m_artistName = resultRow[8].toInt();
}

ServiceTrack::~ServiceTrack()
{
    delete m_provider;
}

void
ServiceTrack::setId(int id)
{
    m_id = id;
}

int
ServiceTrack::id() const
{
    return m_id;
}

void
ServiceTrack::setAlbumId(int albumId)
{
    m_albumId = albumId;
}

int
ServiceTrack::albumId() const
{
    return m_albumId;
}

void
ServiceTrack::setArtistId(int id)
{
    m_artistId = id;
}

int
ServiceTrack::artistId() const
{
    return m_artistId;
}

QString
ServiceTrack::name() const
{
    return m_name;
}

QString
ServiceTrack::prettyName() const
{
    return name();
}

KUrl
ServiceTrack::playableUrl() const
{
    KUrl url( m_playableUrl );
    return url;
}

KUrl
ServiceTrack::downloadableUrl() const
{
    KUrl url( m_downloadableUrl );
    return url;
}

QString
ServiceTrack::uidUrl() const
{
    return m_playableUrl;
}

QString
ServiceTrack::prettyUrl() const
{
    return m_displayUrl;
}

void
ServiceTrack::setUidUrl(const QString & url)
{
    m_playableUrl = url;
    m_displayUrl = url;
    m_downloadableUrl = url;
}

void
ServiceTrack::setDownloadableUrl(const QString & url)
{
    m_downloadableUrl = url;
}

bool
ServiceTrack::isPlayable() const
{
    return true;
}

bool
ServiceTrack::isEditable() const
{
    return false;
}

AlbumPtr
ServiceTrack::album() const
{
    if ( !m_album == 0 )
        return AlbumPtr::staticCast( m_album );
    else
        //FIXME: always return the same default object. this applies to the other methods too
        return Meta::AlbumPtr( new Meta::DefaultAlbum() );
}

ArtistPtr
ServiceTrack::artist() const
{
    if ( !m_artist == 0 )
        return ArtistPtr::staticCast( m_artist );
    else
        return Meta::ArtistPtr( new Meta::DefaultArtist() );
}

GenrePtr
ServiceTrack::genre() const
{
    if ( !m_genre == 0 )
        return GenrePtr::staticCast( m_genre );
    else
        return Meta::GenrePtr( new DefaultGenre() );
}

ComposerPtr
ServiceTrack::composer() const
{
    if ( !m_composer == 0 )
        return ComposerPtr::staticCast( m_composer );
    else
        return Meta::ComposerPtr( new DefaultComposer() );
}

YearPtr
ServiceTrack::year() const
{
    if( m_year )
        return YearPtr::staticCast( m_year );
    else
        return Meta::YearPtr( new DefaultYear() );
}

void
ServiceTrack::setAlbum( const QString &newAlbum )
{
    Q_UNUSED( newAlbum )
}

void
ServiceTrack::setArtist( const QString &newArtist )
{
    Q_UNUSED( newArtist )
}

void
ServiceTrack::setComposer( const QString &newComposer )
{
    Q_UNUSED( newComposer )
}

void
ServiceTrack::setGenre( const QString &newGenre )
{
    Q_UNUSED( newGenre )
}

void
ServiceTrack::setYear( const QString &newYear )
{
    Q_UNUSED( newYear )
}

/* At time of writing, the services (Jamendo & Magnatune.com) I've seen can't handle BPM yet. */
qreal
ServiceTrack::bpm() const
{
    return -1.0;
}

QString
ServiceTrack::comment() const
{
    return QString();
}

void
ServiceTrack::setComment( const QString &newComment )
{
    Q_UNUSED( newComment )
}

QString
ServiceTrack::description() const
{
    return m_description;
}

void
ServiceTrack::setDescription( const QString &newDescription )
{
    m_description = newDescription;
}

double
ServiceTrack::score() const
{
    if( m_provider )
        return m_provider->score();
    else
        return 0.0;
}

void
ServiceTrack::setScore( double newScore )
{
    if( m_provider )
        m_provider->setScore( newScore );
}

int
ServiceTrack::rating() const
{
    if( m_provider )
        return m_provider->rating();
    else
        return 0;
}

void
ServiceTrack::setRating( int newRating )
{
    if( m_provider )
        m_provider->setRating( newRating );
}

qint64
ServiceTrack::length() const
{
    return m_length;
}

int
ServiceTrack::filesize() const
{
    return 0;
}

int
ServiceTrack::sampleRate() const
{
    return 0;
}

int
ServiceTrack::bitrate() const
{
    return 0;
}

int
ServiceTrack::trackNumber() const
{
    return m_trackNumber;
}

void
ServiceTrack::setTrackNumber( int newTrackNumber )
{
    m_trackNumber = newTrackNumber;
}

int
ServiceTrack::discNumber() const
{
    return 0;
}

void
ServiceTrack::setDiscNumber( int newDiscNumber )
{
    Q_UNUSED( newDiscNumber )
}

int
ServiceTrack::playCount() const
{
    if( m_provider )
        return m_provider->playCount();
    else
        return 0;
}

uint
ServiceTrack::lastPlayed() const
{
    if( m_provider )
        return m_provider->lastPlayed().toTime_t();
    else
        return 0;
}

uint
ServiceTrack::firstPlayed() const
{
    if( m_provider )
        return m_provider->firstPlayed().toTime_t();
    else
        return 0;
}

void
ServiceTrack::finishedPlaying( double playedFraction )
{
    if( m_provider )
    {
        m_provider->played( playedFraction, TrackPtr( this ) );
        notifyObservers();
    }
}

QString
ServiceTrack::type() const
{
//     return m_type;
    return Amarok::extension( uidUrl() );
}

void
ServiceTrack::setAlbumPtr( AlbumPtr album )
{
    m_album = album;
}

void
ServiceTrack::setArtist( ArtistPtr artist )
{
    m_artist = artist;
}

void
ServiceTrack::setGenre( GenrePtr genre )
{
    m_genre = genre;
}

void
ServiceTrack::setComposer( ComposerPtr composer )
{
    m_composer = composer;
}

void
ServiceTrack::setYear( YearPtr year )
{
    m_year = year;
}

void
ServiceTrack::setStatisticsProvider( Statistics::StatisticsProvider *provider )
{
    if( m_provider )
        delete m_provider;

    m_provider = provider;
}

void
ServiceTrack::setTitle( const QString &title )
{
    m_name = title;
}

void
ServiceTrack::setLength( qint64 length )
{
    m_length = length;
}


void ServiceTrack::processInfoOf(InfoParserBase * infoParser)
{
    infoParser->getInfo( TrackPtr( this ) );
}


//ServiceArtist
ServiceArtist::ServiceArtist( const QString & name )
    : Meta::Artist()
    , ServiceDisplayInfoProvider()
    , CustomActionsProvider()
    , SourceInfoProvider()
    , BookmarkThisProvider()
    , m_id( 0 )
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

ServiceArtist::ServiceArtist(const QStringList & resultRow)
    : Meta::Artist()
    , ServiceDisplayInfoProvider()
    , CustomActionsProvider()
    , SourceInfoProvider()
    , BookmarkThisProvider()
    , m_tracks()
{

    m_id = resultRow[0].toInt();
    m_name = resultRow[1];
    m_description = resultRow[2];


}

ServiceArtist::~ServiceArtist()
{
    //nothing to do
}


void
ServiceArtist::setId(int id)
{
    m_id = id;
}

int
ServiceArtist::id() const
{
    return m_id;
}

QString
ServiceArtist::name() const
{
    return m_name;
}

QString
ServiceArtist::prettyName() const
{
    return m_name;
}

void
ServiceArtist::setTitle(const QString & title)
{
    m_name = title;
}

TrackList
ServiceArtist::tracks()
{
    return m_tracks;
}

AlbumList
ServiceArtist::albums()
{
    //TODO
    return Meta::AlbumList();
}

void
ServiceArtist::addTrack( TrackPtr track )
{
    m_tracks.append( track );
}


void
ServiceArtist::setDescription(const QString &description)
{
    m_description = description;
}

QString
ServiceArtist::description() const
{
    return m_description;
}

void
ServiceArtist::processInfoOf(InfoParserBase * infoParser)
{
    infoParser->getInfo( ArtistPtr ( this ) );
}



ServiceAlbum::ServiceAlbum( const QString & name )
    : Meta::Album()
    , ServiceDisplayInfoProvider()
    , CustomActionsProvider()
    , SourceInfoProvider()
    , BookmarkThisProvider()
    , m_id( 0 )
    , m_name( name )
    , m_tracks()
    , m_isCompilation( false )
    , m_albumArtist( 0 )
    , m_artistId( 0 )
{
    //nothing to do
}

ServiceAlbum::ServiceAlbum(const QStringList & resultRow)
    : Meta::Album()
    , ServiceDisplayInfoProvider()
    , CustomActionsProvider()
    , SourceInfoProvider()
    , BookmarkThisProvider()
    , m_id( resultRow[0].toInt() )
    , m_name( resultRow[1] )
    , m_isCompilation( false )
    , m_albumArtist( 0 )
    , m_description( resultRow[2] )
    , m_artistId( resultRow[3].toInt() )
    , m_artistName()
{
}

ServiceAlbum::~ServiceAlbum()
{
    //nothing to do
}

void
ServiceAlbum::setId(int id)
{
    m_id = id;
}

int
ServiceAlbum::id() const
{
    return m_id;
}

void
ServiceAlbum::setArtistId(int artistId)
{
    m_artistId = artistId;
}

int
ServiceAlbum::artistId() const
{
    return m_artistId;
}

void
ServiceAlbum::setArtistName(const QString & name)
{
    m_artistName = name;
}

QString
ServiceAlbum::artistName() const
{
    return m_artistName;
}

QString
ServiceAlbum::name() const
{
    return m_name;
}

QString
ServiceAlbum::prettyName() const
{
    return m_name;
}

void
ServiceAlbum::setTitle(const QString & title)
{
    m_name = title;
}

bool
ServiceAlbum::isCompilation() const
{
    return m_isCompilation;
}

bool
ServiceAlbum::hasAlbumArtist() const
{
    return !m_albumArtist.isNull();
}

ArtistPtr
ServiceAlbum::albumArtist() const
{
    return ArtistPtr::staticCast( m_albumArtist );
}

TrackList
ServiceAlbum::tracks()
{
    return m_tracks;
}

void
ServiceAlbum::addTrack( TrackPtr track )
{
    m_tracks.append( track );
}

void
ServiceAlbum::setAlbumArtist( ArtistPtr artist )
{
    m_albumArtist = artist;
}

void
ServiceAlbum::setIsCompilation( bool compilation )
{
    m_isCompilation = compilation;
}

void ServiceAlbum::setDescription(const QString &description)
{
    m_description = description;
}

QString
ServiceAlbum::description() const
{
    return m_description;
}

void
ServiceAlbum::processInfoOf(InfoParserBase * infoParser)
{
    infoParser->getInfo( AlbumPtr( this ) );
}


//ServiceGenre

ServiceGenre::ServiceGenre( const QString &name )
    : Meta::Genre()
    , ServiceDisplayInfoProvider()
    , CustomActionsProvider()
    , SourceInfoProvider()
    , BookmarkThisProvider()
    , m_id( 0 )
    , m_albumId( 0 )
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

ServiceGenre::ServiceGenre(const QStringList & row)
    : Meta::Genre()
    , ServiceDisplayInfoProvider()
    , CustomActionsProvider()
    , SourceInfoProvider()
    , BookmarkThisProvider()
    , m_id( 0 )
    , m_albumId( 0 )
    , m_name( row[1] )
    , m_tracks()
{
}

ServiceGenre::~ServiceGenre()
{
    //nothing to do
}

void
ServiceGenre::setId(int id)
{
    m_id = id;
}

int
ServiceGenre::id() const
{
    return m_id;
}

QString
ServiceGenre::name() const
{
    return m_name;
}

QString
ServiceGenre::prettyName() const
{
    return m_name;
}

int
ServiceGenre::albumId()
{
    return m_albumId;
}

void
ServiceGenre::setAlbumId(int albumId)
{
    m_albumId = albumId;
}


TrackList
ServiceGenre::tracks()
{
    return m_tracks;
}

void
ServiceGenre::addTrack( TrackPtr track )
{
    m_tracks.append( track );
}

void
ServiceGenre::processInfoOf(InfoParserBase * infoParser)
{
    Q_UNUSED( infoParser );
    return; // do nothing
}

//ServiceComposer

ServiceComposer::ServiceComposer( const QString &name )
    : Meta::Composer()
    , ServiceDisplayInfoProvider()
    , CustomActionsProvider()
    , SourceInfoProvider()
    , BookmarkThisProvider()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

ServiceComposer::~ServiceComposer()
{
    //nothing to do
}

QString
ServiceComposer::name() const
{
    return m_name;
}

QString
ServiceComposer::prettyName() const
{
    return m_name;
}

TrackList
ServiceComposer::tracks()
{
    return m_tracks;
}

void
ServiceComposer::addTrack( ServiceTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}

void
ServiceComposer::processInfoOf(InfoParserBase * infoParser)
{
    Q_UNUSED( infoParser );
    return; // do nothing
}

//ServiceYear

ServiceYear::ServiceYear( const QString &name )
    : Meta::Year()
    , ServiceDisplayInfoProvider()
    , CustomActionsProvider()
    , SourceInfoProvider()
    , BookmarkThisProvider()
    , m_name( name )
    , m_tracks()
{
    //nothing to do
}

ServiceYear::~ServiceYear()
{
    //nothing to do
}

QString
ServiceYear::name() const
{
    return m_name;
}

QString
ServiceYear::prettyName() const
{
    return m_name;
}

TrackList
ServiceYear::tracks()
{
    return m_tracks;
}

void
ServiceYear::addTrack( ServiceTrackPtr track )
{
    m_tracks.append( TrackPtr::staticCast( track ) );
}


void
ServiceYear::processInfoOf(InfoParserBase * infoParser)
{
    Q_UNUSED( infoParser );
    return; // do nothing
}

