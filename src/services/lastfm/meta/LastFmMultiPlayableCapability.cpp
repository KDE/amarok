/****************************************************************************************
 * Copyright (c) 2008 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz>                                      *
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

#include "LastFmMultiPlayableCapability.h"

#include "EngineController.h"

LastFmMultiPlayableCapability::LastFmMultiPlayableCapability( LastFm::Track *track )
    : Capabilities::MultiPlayableCapability()
    , m_url( track->internalUrl() )
    , m_track( track )
{
    connect( track, &LastFm::Track::skipTrack, this, &LastFmMultiPlayableCapability::skip );

    Q_ASSERT( The::mainWindow() );
    connect( The::mainWindow(), &MainWindow::skipTrack, this, &LastFmMultiPlayableCapability::skip );

    // we only update underlying Last.fm metadata once it starts playing, prevent wrong
    // metadata Last.fm submissions etc.
    Q_ASSERT( EngineController::instance() );
    connect( EngineController::instance(), &EngineController::trackPlaying,
             this, &LastFmMultiPlayableCapability::slotTrackPlaying );
}

LastFmMultiPlayableCapability::~LastFmMultiPlayableCapability()
{
}

void
LastFmMultiPlayableCapability::fetchFirst()
{
    DEBUG_BLOCK
    m_tuner = new lastfm::RadioTuner( lastfm::RadioStation( m_track->uidUrl() ) );
    m_tuner->setParent( this ); // memory management

    connect( m_tuner, &lastfm::RadioTuner::trackAvailable, this, &LastFmMultiPlayableCapability::slotNewTrackAvailable );
    connect( m_tuner, &lastfm::RadioTuner::error, this, &LastFmMultiPlayableCapability::error );
}

void
LastFmMultiPlayableCapability::fetchNext()
{
    DEBUG_BLOCK
    m_currentTrack = m_tuner->takeNextTrack();
    Q_EMIT playableUrlFetched( m_currentTrack.url() );
}

void
LastFmMultiPlayableCapability::slotTrackPlaying( const Meta::TrackPtr &track )
{
    // time to update underlying track with metadata
    // warning: this depends on MetaProxy::Track operator== returning true
    // between proxy and underlying track!
    if( track == m_track )
        m_track->setTrackInfo( m_currentTrack );
}

void
LastFmMultiPlayableCapability::slotNewTrackAvailable()
{
    DEBUG_BLOCK
    if( m_currentTrack.isNull() ) // we only force a track change at the beginning
    {
        fetchNext();
        // we update metadata immediately for the very first track
        m_track->setTrackInfo( m_currentTrack );
    }
}

void
LastFmMultiPlayableCapability::skip()
{
    DEBUG_BLOCK
    fetchNext();
}

void
LastFmMultiPlayableCapability::error( lastfm::ws::Error e )
{
    // last.fm is returning an AuthenticationFailed message when the user is not
    // a subscriber, even if the credentials are OK
    if( e == lastfm::ws::SubscribersOnly || e == lastfm::ws::AuthenticationFailed )
    {
        Amarok::Logger::longMessage( i18n( "To listen to Last.fm streams "
                "and radio you need to be a paying Last.fm subscriber and you need to "
                "stream from a <a href='http://www.last.fm/announcements/radio2013'>supported "
                "country</a>. All other Last.fm features work fine." ) );
    }
    else
    {
        Amarok::Logger::longMessage(
                    i18n( "Error starting track from Last.fm radio" ) );
    }
}
