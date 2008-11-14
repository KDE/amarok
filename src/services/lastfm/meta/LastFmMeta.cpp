/*
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
   Copyright (C) 2008 Shane King <kde@dontletsstart.com>
   Copyright (C) 2008 Leo Franchi <lfranchi@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "LastFmMeta.h"
#include "LastFmMeta_p.h"
#include "LastFmMeta_p.moc"
#include "LastFmCapabilityImpl_p.h"
#include "LastFmCapabilityImpl_p.moc"
#include "MultiPlayableCapabilityImpl_p.h"
#include "MultiPlayableCapabilityImpl_p.moc"
#include "CurrentTrackActionsCapabilityImpl_p.h"
#include "CurrentTrackActionsCapabilityImpl_p.moc"
#include "ServiceSourceInfoCapability.h"

#include "LastFmService.h"
#include "LastFmStreamInfoCapability.h"
#include "ScrobblerAdapter.h"

#include "Debug.h"

#include <QPointer>

#include <KSharedPtr>
#include <KStandardDirs>

#include <lastfm/ws/WsKeys.h>
#include <lastfm/ws/WsReply.h>
#include <lastfm/ws/WsRequestBuilder.h>
#include <lastfm/types/Track.h>

namespace LastFm {

class LastFmArtist;
class LastFmAlbum;
class LastFmGenre;
class LastFmComposer;
class LastFmYear;

Track::Track( const QString &lastFmUri )
    : QObject()
    , Meta::Track()
    , d( new Private() )
{
    d->lastFmUri = QUrl( lastFmUri );
    d->t = this;
    

    init();
}

Track::Track( ::Track track )
    : QObject()
    , Meta::Track()
    , d( new Private() )
{
    d->t = this;
    d->track = track.title();
    d->lastFmTrack = track;
    WsReply* reply = WsRequestBuilder( "track.getInfo" )
      .add( "artist", track.artist() )
      .add( "track", track.title() )
      .add( "api_key", Ws::ApiKey )
      .get();
      
      connect( reply, SIGNAL( finished( WsReply* ) ), SLOT( slotResultReady( WsReply* ) ) );
}


Track::~Track()
{
    delete d;
}

void Track::init( int id /* = -1*/ )
{
    if( id != -1 )
        d->lastFmUri = QUrl( "lastfm://play/tracks/" + QString::number( id ) );
    d->length = 0;

    d->albumPtr = Meta::AlbumPtr( new LastFmAlbum( QPointer<Track::Private>( d ) ) );
    d->artistPtr = Meta::ArtistPtr( new LastFmArtist( QPointer<Track::Private>( d ) ) );
    d->genrePtr = Meta::GenrePtr( new LastFmGenre( QPointer<Track::Private>( d ) ) );
    d->composerPtr = Meta::ComposerPtr( new LastFmComposer( QPointer<Track::Private>( d ) ) );
    d->yearPtr = Meta::YearPtr( new LastFmYear( QPointer<Track::Private>( d ) ) );

    PopupDropperAction * loveAction = new PopupDropperAction( KIcon( "emblem-favorite-amarok" ), i18n( "Last.fm: &Love" ), this );
    loveAction->setShortcut( i18n( "Ctrl+L" ) );
    loveAction->setStatusTip( i18n( "Love this track on Last.fm" ) );
    connect( loveAction, SIGNAL( triggered() ), this, SLOT( love() ) );
    m_currentTrackActions.append( loveAction );

    PopupDropperAction * banAction = new PopupDropperAction( KIcon( "amarok_remove" ), i18n( "Last.fm: &Ban" ), this );
    banAction->setShortcut( i18n( "Ctrl+B" ) );
    banAction->setStatusTip( i18n( "Ban this track" ) );
    connect( banAction, SIGNAL( triggered() ), this, SLOT( ban() ) );
    m_currentTrackActions.append( banAction );

    PopupDropperAction * skipAction = new PopupDropperAction( KIcon( "media-seek-forward-amarok" ), i18n( "Last.fm: &Skip" ), this );
    skipAction->setShortcut( i18n( "Ctrl+S" ) );
    skipAction->setStatusTip( i18n( "Skip this track" ) );
    connect( skipAction, SIGNAL( triggered() ), this, SLOT( skip() ) );
    m_currentTrackActions.append( skipAction );
}

QString
Track::name() const
{
    if( d->track.isEmpty() )
    {
        return streamName();
    }
    else
    {
        return d->track;
    }
}

QString
Track::prettyName() const
{
    return name();
}

QString
Track::fullPrettyName() const
{
    if( d->track.isEmpty() || d->artist.isEmpty() )
        return prettyName();
    else
        return i18n("%1 - %2", d->artist, d->track );
}

QString
Track::sortableName() const
{
    // TODO
    return name();
}

QString LastFm::Track::fixedName() const
{

    //unless this is a stream for a single track, return the generic name
    if ( streamName() != i18n( "Track Radio" ) )
        return streamName();
    else
        return name();
}


KUrl
Track::playableUrl() const
{
    return d->lastFmUri.toString();
}

KUrl
Track::internalUrl() const
{
    return KUrl( d->trackPath );
}

QString
Track::prettyUrl() const
{
    return d->lastFmUri.toString();
}

QString
Track::uidUrl() const
{
    return d->lastFmUri.toString();
}

bool
Track::isPlayable() const
{
    //we could check connectivity here...
    return !d->trackPath.isEmpty();
}

Meta::AlbumPtr
Track::album() const
{
    return d->albumPtr;
}

Meta::ArtistPtr
Track::artist() const
{
    return d->artistPtr;
}

Meta::GenrePtr
Track::genre() const
{
    return d->genrePtr;
}

Meta::ComposerPtr
Track::composer() const
{
    return d->composerPtr;
}

Meta::YearPtr
Track::year() const
{
    return d->yearPtr;
}

QString
Track::comment() const
{
    return QString();
}

double
Track::score() const
{
    return 0.0;
}

void
Track::setScore( double newScore )
{
    Q_UNUSED( newScore ); //stream
}

int
Track::rating() const
{
    return 0;
}

void
Track::setRating( int newRating )
{
    Q_UNUSED( newRating ); //stream
}

int
Track::trackNumber() const
{
    return 0;
}

int
Track::discNumber() const
{
    return 0;
}

int
Track::length() const
{
    return d->length;
}

int
Track::filesize() const
{
    return 0; //stream
}

int
Track::sampleRate() const
{
    return 0; //does the engine deliver this?
}

int
Track::bitrate() const
{
    return 0; //does the engine deliver this??
}

uint
Track::lastPlayed() const
{
    return 0; //TODO do we need this?
}

int
Track::playCount() const
{
    return 0; //TODO do we need this?
}

QString
Track::type() const
{
    return "stream/lastfm";
}
void
Track::finishedPlaying( double playedFraction )
{
    Q_UNUSED( playedFraction );
    //TODO
}

bool
Track::inCollection() const
{
    return false;
}

Collection*
Track::collection() const
{
    return 0;
}

void 
Track::setTrackInfo( const ::Track &track )
{
    d->setTrackInfo( track );
}

QString
Track::streamName() const
{
    // parse the url to get a name if we don't have a track name (ie we're not playing the station)
    // do it as name rather than prettyname so it shows up nice in the playlist.
    QStringList elements = d->lastFmUri.toString().split( '/', QString::SkipEmptyParts );
    if( elements.size() >= 2 && elements[0] == "lastfm:" )
    {
        QString customPart = elements[2];
        customPart = customPart.replace( "%20", " " );
        
        
        if( elements[1] == "globaltags" )
        {
                // lastfm://globaltag/<tag>
            if( elements.size() >= 3 )
                return i18n( "Global Tag Radio: \"%1\"", customPart );
        }
        else if( elements[1] == "usertags" )
        {
                // lastfm://usertag/<tag>
            if( elements.size() >= 3 )
                return i18n( "User Tag Radio: \"%1\"", customPart );
        }
        else if( elements[1] == "artist" )
        {
            if( elements.size() >= 4 )
            {
                    // lastfm://artist/<artist>/similarartists
                if( elements[3] == "similarartists" )
                    return i18n( "Similar Artists to \"%1\"", customPart );

                    // lastfm://artist/<artist>/fans
                else if( elements[3] == "fans" )
                    return i18n( "Artist Fan Radio: \"%1\"", customPart );
            }
        }
        else if( elements[1] == "user" )
        {
            if( elements.size() >= 4 )
            {
                // lastfm://user/<user>/neighbours
                if( elements[3] == "neighbours" )
                    return i18n( "%1's Neighbor Radio", elements[2] );
                    
                // lastfm://user/<user>/personal
                else if( elements[3] == "personal" )
                    return i18n( "%1's Personal Radio", elements[2] );
                    
                // lastfm://user/<user>/loved
                else if( elements[3] == "loved" )
                    return i18n( "%1's Loved Radio", elements[2] );
                    
                // lastfm://user/<user>/recommended/<popularity>
                else if( elements.size() >= 5 && elements[3] == "recommended" )
                    return i18n( "%1's Recommended Radio (Popularity %2)", elements[2], elements[4] );
            }
        }
        else if( elements[1] == "group" )
        {
                // lastfm://group/<group>
            if( elements.size() >= 3 )
                return i18n( "Group Radio: %1", elements[2] );
        }
        else if( elements[1] == "play" )
        {
            if( elements.size() >= 4 )
            {
                    // lastfm://play/tracks/<track #s>
                if ( elements[2] == "tracks" )
                    return i18n( "Track Radio" );

                    // lastfm://play/artists/<artist #s>
                else if ( elements[2] == "artists" )
                    return i18n( "Artist Radio" );
            }
        }
    }

    return d->lastFmUri.toString();
}

void
Track::love()
{
    DEBUG_BLOCK
    
    debug() << "info:" << d->lastFmTrack.artist() << d->lastFmTrack.title();
    WsReply* reply = MutableTrack( d->lastFmTrack ).love();
    connect( reply, SIGNAL( finished( WsReply* ) ), this, SLOT( slotWsReply( WsReply* ) ) );
}

void
Track::ban()
{
    DEBUG_BLOCK
    WsReply* reply = MutableTrack( d->lastFmTrack ).ban();
    connect( reply, SIGNAL( finished( WsReply* ) ), this, SLOT( slotWsReply( WsReply* ) ) );
    emit( skipTrack() );
    
}

void
Track::skip()
{
    DEBUG_BLOCK
    //MutableTrack( d->lastFmTrack ).skip();
    emit( skipTrack() );
}

void Track::slotResultReady( WsReply *reply )
{
    if( reply->error() == Ws::NoError )
    {
        QString id = reply->lfm()[ "track" ][ "id" ].nonEmptyText();
        QString streamable = reply->lfm()[ "track" ][ "streamable" ].nonEmptyText();
        if( streamable.toInt() == 1 )
            init( id.toInt() );
        else
            init();
    } else
    {
        init();
    }
}


void 
Track::slotWsReply( WsReply *reply )
{
    if( reply->error() == Ws::NoError )
    {   
        //debug() << "successfully completed WS transaction";
    } else
    {
        debug() << "ERROR in last.fm skip or ban!" << reply->error();
    }
}

bool
Track::hasCapabilityInterface( Meta::Capability::Type type ) const
{
    return type == Meta::Capability::LastFm
                || type == Meta::Capability::MultiPlayable
                || type == Meta::Capability::SourceInfo
                || type == Meta::Capability::CurrentTrackActions
                || type == Meta::Capability::StreamInfo;
}

Meta::Capability*
Track::asCapabilityInterface( Meta::Capability::Type type )
{
    switch( type )
    {
        case Meta::Capability::LastFm:
            return new LastFmCapabilityImpl( this );
        case Meta::Capability::MultiPlayable:
            return new MultiPlayableCapabilityImpl( this );
        case Meta::Capability::SourceInfo:
            return new ServiceSourceInfoCapability( this );
        case Meta::Capability::CurrentTrackActions:
            return new CurrentTrackActionsCapabilityImpl( this );
        case Meta::Capability::StreamInfo:
            return new LastFmStreamInfoCapability( this );
        default:
            return 0;
    }
}

} // namespace LastFm

QString LastFm::Track::sourceName()
{
    return "Last.fm";
}

QString LastFm::Track::sourceDescription()
{
    return i18n( "Last.fm is cool..." );
}

QPixmap LastFm::Track::emblem()
{
    if (  !d->track.isEmpty() )
        return QPixmap( KStandardDirs::locate( "data", "amarok/images/emblem-lastfm.png" ) );
    else
        return QPixmap();
}

QList< PopupDropperAction * > LastFm::Track::nowPlayingActions() const
{
    return m_currentTrackActions;
}

#include "LastFmMeta.moc"



