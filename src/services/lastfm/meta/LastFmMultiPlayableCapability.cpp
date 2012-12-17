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

LastFmMultiPlayableCapability::LastFmMultiPlayableCapability( LastFm::Track *track )
    : Capabilities::MultiPlayableCapability()
    , m_url( track->internalUrl() )
    , m_track( track )
    , m_currentTrack( lastfm::Track() )
{
    Meta::TrackPtr trackptr( track );
    subscribeTo( trackptr );

    connect( track, SIGNAL(skipTrack()), this, SLOT(skip()) );
    connect( The::mainWindow(), SIGNAL(skipTrack()), SLOT(skip()) );
}

LastFmMultiPlayableCapability::~LastFmMultiPlayableCapability()
{
}

void
LastFmMultiPlayableCapability::fetchFirst()
{
    DEBUG_BLOCK
    m_tuner = new lastfm::RadioTuner( lastfm::RadioStation( m_track->uidUrl() ) );
    m_tuner->setParent( this );

    connect( m_tuner, SIGNAL( trackAvailable() ), this, SLOT( slotNewTrackAvailable() ) );
    connect( m_tuner, SIGNAL( error(lastfm::ws::Error,QString) ), this, SLOT( error( lastfm::ws::Error ) ) );
}

void
LastFmMultiPlayableCapability::fetchNext()
{
    DEBUG_BLOCK
    m_currentTrack = m_tuner->takeNextTrack();
    m_track->setTrackInfo( m_currentTrack );
}

void
LastFmMultiPlayableCapability::metadataChanged( Meta::TrackPtr track )
{
    const LastFm::TrackPtr ltrack = LastFm::TrackPtr::dynamicCast( track );

    if( ltrack.isNull() )
        return;

    KUrl url = ltrack->internalUrl();
    if( url.isEmpty() || url != m_url ) // always should let empty url through, since otherwise we swallow an error getting first track
    {
        debug() << __PRETTY_FUNCTION__ << "url changed, informing EngineController";
        m_url = url;
        emit playableUrlFetched( url );
    }
}

void
LastFmMultiPlayableCapability::slotNewTrackAvailable()
{
    DEBUG_BLOCK
    if( m_currentTrack.isNull() ) // we only force a track change at the beginning
    {
        m_currentTrack = m_tuner->takeNextTrack();
        m_track->setTrackInfo( m_currentTrack );
    }
}

void
LastFmMultiPlayableCapability::skip()
{
    DEBUG_BLOCK
    fetchNext();
    /* fetchNext() calls m_track->setTrackInfo()
     * Track::setTrackInfo() calls Track::Private::setTrackInfo()
     * Track::Private::setTrackInfo() calls Meta::Base::notifyObservers()
     * Meta::Base::notifyObservers() calls *OUR* metadataChanged()
     * OUR metadataChanged() emits playableUrlFetched( url )
     * playableUrlFetched( url ) is caught by EngineController. */
}

void
LastFmMultiPlayableCapability::error( lastfm::ws::Error e )
{
    if( e == lastfm::ws::SubscribersOnly || e == lastfm::ws::AuthenticationFailed )
    {   // last.fm is returning an AuthenticationFailed message when the user is not a subscriber, even if the credentials are OK
        Amarok::Components::logger()->longMessage(
            i18n( "To listen to this stream you need to be a paying Last.fm subscriber. "
                  "All the other Last.fm features are unaffected." ) );
    }
    else
    {
        Amarok::Components::logger()->longMessage(
                    i18n( "Error starting track from Last.fm radio" ) );
    }
}
