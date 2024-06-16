/****************************************************************************************
 * Copyright (c) 2007 Shane King <kde@dontletsstart.com>                                *
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2012 Matěj Laitl <matej@laitlcz>                                       *
 * Copyright (c) 2013 Vedant Agarwala <vedant.kota@gmail.com>                           *
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

#define DEBUG_PREFIX "lastfm"

#include "ScrobblerAdapter.h"

#include "MainWindow.h"
#include "core/collections/Collection.h"
#include "core/logger/Logger.h"
#include "core/meta/Meta.h"
#include "core/meta/support/MetaConstants.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"

#include <KLocalizedString>

#include <QNetworkReply>

#include <misc.h>

ScrobblerAdapter::ScrobblerAdapter( const QString &clientId, const LastFmServiceConfigPtr &config )
    : m_scrobbler( clientId )
    , m_config( config )
{
    // work around a bug in liblastfm -- -it doesn't create its config dir, so when it
    // tries to write the track cache, it fails silently. Last check: liblastfm 1.0.!
    QList<QDir> dirs;
    dirs << lastfm::dir::runtimeData() << lastfm::dir::cache() << lastfm::dir::logs();
    for( const QDir &dir : dirs )
    {
        if( !dir.exists() )
        {
            debug() << "creating" << dir.absolutePath() << "directory for liblastfm";
            dir.mkpath( "." );
        }
    }

    connect( The::mainWindow(), &MainWindow::loveTrack,
             this, &ScrobblerAdapter::loveTrack );
    connect( The::mainWindow(), &MainWindow::banTrack,
             this, &ScrobblerAdapter::banTrack );

    connect( &m_scrobbler, &lastfm::Audioscrobbler::scrobblesSubmitted,
             this, &ScrobblerAdapter::slotScrobblesSubmitted );
    connect( &m_scrobbler, &lastfm::Audioscrobbler::nowPlayingError,
             this, &ScrobblerAdapter::slotNowPlayingError );
}

ScrobblerAdapter::~ScrobblerAdapter()
{
}

QString
ScrobblerAdapter::prettyName() const
{
    return i18n( "Last.fm" );
}

StatSyncing::ScrobblingService::ScrobbleError
ScrobblerAdapter::scrobble( const Meta::TrackPtr &track, double playedFraction,
                            const QDateTime &time )
{
    Q_ASSERT( track );
    if( isToBeSkipped( track ) )
    {
        debug() << "scrobble(): refusing track" << track->prettyUrl()
                << "- contains label:" << m_config->filteredLabel() << "which is marked to be skipped";
        return SkippedByUser;
    }
    if( track->length() * qMin( 1.0, playedFraction ) < 30 * 1000 )
    {
        debug() << "scrobble(): refusing track" << track->prettyUrl() << "- played time ("
                << track->length() / 1000 << "*" << playedFraction << "s) shorter than 30 s";
        return TooShort;
    }
    int playcount = qRound( playedFraction );
    if( playcount <= 0 )
    {
        debug() << "scrobble(): refusing track" << track->prettyUrl() << "- played "
                << "fraction (" << playedFraction * 100 << "%) less than 50 %";
        return TooShort;
    }

    lastfm::MutableTrack lfmTrack;
    copyTrackMetadata( lfmTrack, track );
    // since liblastfm >= 1.0.3 it interprets following extra property:
    lfmTrack.setExtra( "playCount", QString::number( playcount ) );
    lfmTrack.setTimeStamp( time.isValid() ? time : QDateTime::currentDateTime() );
    debug() << "scrobble: " << lfmTrack.artist() << "-" << lfmTrack.album() << "-"
            << lfmTrack.title() << "source:" << lfmTrack.source() << "duration:"
            << lfmTrack.duration();
    m_scrobbler.cache( lfmTrack );
    m_scrobbler.submit(); // since liblastfm 1.0.7, submit() is not called automatically upon cache()
    switch( lfmTrack.scrobbleStatus() )
    {
        case lastfm::Track::Cached:
        case lastfm::Track::Submitted:
            return NoError;
        case lastfm::Track::Null:
        case lastfm::Track::Error:
            break;
    }
    return BadMetadata;
}

void
ScrobblerAdapter::updateNowPlaying( const Meta::TrackPtr &track )
{
    lastfm::MutableTrack lfmTrack;
    if( track )
    {
        if( isToBeSkipped( track ) )
        {
            debug() << "updateNowPlaying(): refusing track" << track->prettyUrl()
                    << "- contains label:" << m_config->filteredLabel() << "which is marked to be skipped";
            return;
        }
        copyTrackMetadata( lfmTrack, track );
        debug() << "nowPlaying: " << lfmTrack.artist() << "-" << lfmTrack.album() << "-"
                << lfmTrack.title() << "source:" << lfmTrack.source() << "duration:"
                << lfmTrack.duration();
        m_scrobbler.nowPlaying( lfmTrack );
    }
    else
    {
        debug() << "removeNowPlaying";
        QNetworkReply *reply = lfmTrack.removeNowPlaying(); // works even with empty lfmTrack
        connect( reply, &QNetworkReply::finished, reply, &QNetworkReply::deleteLater ); // don't leak
    }
}

void
ScrobblerAdapter::loveTrack( const Meta::TrackPtr &track ) // slot
{
    if( !track )
        return;

    lastfm::MutableTrack trackInfo;
    copyTrackMetadata( trackInfo, track );
    trackInfo.love();
    Amarok::Logger::shortMessage( i18nc( "As in Last.fm", "Loved Track: %1", track->prettyName() ) );
}

void
ScrobblerAdapter::banTrack( const Meta::TrackPtr &track ) // slot
{
    if( !track )
        return;

    lastfm::MutableTrack trackInfo;
    copyTrackMetadata( trackInfo, track );
    trackInfo.ban();
    Amarok::Logger::shortMessage( i18nc( "As in Last.fm", "Banned Track: %1", track->prettyName() ) );
}

void
ScrobblerAdapter::slotScrobblesSubmitted( const QList<lastfm::Track> &tracks )
{
    for( const lastfm::Track &track : tracks )
    {
        switch( track.scrobbleStatus() )
        {
            case lastfm::Track::Null:
                warning() << "slotScrobblesSubmitted(): track" << track
                          << "has Null scrobble status, strange";
                break;
            case lastfm::Track::Cached:
                warning() << "slotScrobblesSubmitted(): track" << track
                          << "has Cached scrobble status, strange";
                break;
            case lastfm::Track::Submitted:
                if( track.corrected() && m_config->announceCorrections() )
                    announceTrackCorrections( track );
                break;
            case lastfm::Track::Error:
                warning() << "slotScrobblesSubmitted(): error scrobbling track" << track
                          << ":" << track.scrobbleErrorText();
                break;
        }
    }
}

void
ScrobblerAdapter::slotNowPlayingError( int code, const QString &message )
{
    Q_UNUSED( code )
    warning() << "error updating Now Playing status:" << message;
}

void
ScrobblerAdapter::copyTrackMetadata( lastfm::MutableTrack &to, const Meta::TrackPtr &track )
{
    to.setTitle( track->name() );

    QString artistOrComposer;
    Meta::ComposerPtr composer = track->composer();
    if( m_config->scrobbleComposer() && composer )
        artistOrComposer = composer->name();
    Meta::ArtistPtr artist = track->artist();
    if( artistOrComposer.isEmpty() && artist )
        artistOrComposer = artist->name();
    to.setArtist( artistOrComposer );

    Meta::AlbumPtr album = track->album();
    Meta::ArtistPtr albumArtist;
    if( album )
    {
        to.setAlbum( album->name() );
        albumArtist = album->hasAlbumArtist() ? album->albumArtist() : Meta::ArtistPtr();
    }
    if( albumArtist )
        to.setAlbumArtist( albumArtist->name() );

    to.setDuration( track->length() / 1000 );
    if( track->trackNumber() >= 0 )
        to.setTrackNumber( track->trackNumber() );

    lastfm::Track::Source source = lastfm::Track::Player;
    if( track->type() == "stream/lastfm" )
        source = lastfm::Track::LastFmRadio;
    else if( track->type().startsWith( "stream" ) )
        source = lastfm::Track::NonPersonalisedBroadcast;
    else if( track->collection() && track->collection()->collectionId() != "localCollection" )
        source = lastfm::Track::MediaDevice;
    to.setSource( source );
}

static QString
printCorrected( qint64 field, const QString &original, const QString &corrected )
{
    if( corrected.isEmpty() || original == corrected )
        return QString();
    return i18nc( "%1 is field name such as Album Name; %2 is the original value; %3 is "
                  "the corrected value", "%1 <b>%2</b> should be corrected to "
                  "<b>%3</b>", Meta::i18nForField( field ), original, corrected );
}

static QString
printCorrected( qint64 field, const lastfm::AbstractType &original, const lastfm::AbstractType &corrected )
{
    return printCorrected( field, original.toString(), corrected.toString() );
}

void
ScrobblerAdapter::announceTrackCorrections( const lastfm::Track &track )
{
    static const lastfm::Track::Corrections orig = lastfm::Track::Original;
    static const lastfm::Track::Corrections correct = lastfm::Track::Corrected;

    QString trackName = i18nc( "%1 is artist, %2 is title", "%1 - %2",
                               track.artist().name(), track.title() );
    QStringList lines;
    lines << i18n( "Last.fm suggests that some tags of track <b>%1</b> should be "
                   "corrected:", trackName );
    QString line;
    line = printCorrected( Meta::valTitle, track.title( orig ), track.title( correct ) );
    if( !line.isEmpty() )
        lines << line;
    line = printCorrected( Meta::valAlbum, track.album( orig ), track.album( correct ) );
    if( !line.isEmpty() )
        lines << line;
    line = printCorrected( Meta::valArtist, track.artist( orig ), track.artist( correct ) );
    if( !line.isEmpty() )
        lines << line;
    line = printCorrected( Meta::valAlbumArtist, track.albumArtist( orig ), track.albumArtist( correct ) );
    if( !line.isEmpty() )
        lines << line;
    Amarok::Logger::longMessage( lines.join( "<br>" ) );
}

bool
ScrobblerAdapter::isToBeSkipped( const Meta::TrackPtr &track ) const
{
    Q_ASSERT( track );
    if( !m_config->filterByLabel() )
        return false;
    for( const Meta::LabelPtr &label : track->labels() )
        if( label->name() == m_config->filteredLabel() )
            return true;
    return false;
}
