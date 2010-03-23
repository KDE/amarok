/****************************************************************************************
 * Copyright (c) 2008 Shane King <kde@dontletsstart.com>                                *
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

#ifndef AMAROK_MULTIPLAYABLECAPABILITYIMPL_P_H
#define AMAROK_MULTIPLAYABLECAPABILITYIMPL_P_H

#include "Debug.h"
#include "LastFmMeta.h"
#include "core/meta/Meta.h"
#include "core/capabilities/MultiPlayableCapability.h"
#include "StatusBar.h"

#include <lastfm/Track>
#include <lastfm/RadioTuner>
#include <lastfm/ws.h>


class MultiPlayableCapabilityImpl : public Capabilities::MultiPlayableCapability, public Meta::Observer
{
    Q_OBJECT
    public:
        MultiPlayableCapabilityImpl( LastFm::Track *track )
            : Capabilities::MultiPlayableCapability()
            , m_url( track->internalUrl() )
            , m_track( track )
            , m_currentTrack( lastfm::Track() )
        {
            Meta::TrackPtr trackptr( track );
            subscribeTo( trackptr );
            
            connect( track, SIGNAL( skipTrack() ), this, SLOT( skip() ) );
            connect( The::mainWindow(), SIGNAL( skipTrack() ), SLOT( skip() ) );
        }

        virtual ~MultiPlayableCapabilityImpl() 
        {}

        virtual void fetchFirst()
        {
            DEBUG_BLOCK
            m_tuner = new lastfm::RadioTuner( lastfm::RadioStation( m_track->uidUrl() ) );
            
            connect( m_tuner, SIGNAL( trackAvailable() ), this, SLOT( slotNewTrackAvailable() ) );
            connect( m_tuner, SIGNAL( error( lastfm::ws::Error ) ), this, SLOT( error( lastfm::ws::Error ) ) );
        }
        
        virtual void fetchNext()
        {
            DEBUG_BLOCK
            m_currentTrack = m_tuner->takeNextTrack();
            m_track->setTrackInfo( m_currentTrack );

        }
        
        using Observer::metadataChanged;
        virtual void metadataChanged( Meta::TrackPtr track )
        {
            const LastFm::TrackPtr ltrack = LastFm::TrackPtr::dynamicCast( track );
            
            if( ltrack.isNull() )
                return;

            KUrl url = ltrack->internalUrl();
            if( url.isEmpty() || url != m_url ) // always should let empty url through, since otherwise we swallow an error getting first track
            {
                m_url = url;
                emit playableUrlFetched( url );
            }
        }

    public slots:

        void slotNewTrackAvailable()
        {
            if( m_currentTrack.isNull() ) // we only force a track change at the beginning
            {
                m_currentTrack = m_tuner->takeNextTrack();
                m_track->setTrackInfo( m_currentTrack );
            }
        }
        
        virtual void skip()
        {
            fetchNext();
            // now we force a new signal to be emitted to kick the enginecontroller to moving on
            //KUrl url = m_track->playableUrl();
            //emit playableUrlFetched( url );
        }
        
        void error( lastfm::ws::Error e )
        {
            if( e == lastfm::ws::SubscribersOnly || e == lastfm::ws::AuthenticationFailed )
            {   // last.fm is returning an AuthenticationFailed message when the user is not a subscriber, even if the credentials are OK
                The::statusBar()->shortMessage( i18n( "To listen to this stream you need to be a paying Last.Fm subscriber. All the other Last.Fm features are unaffected." ) );
            } else {
                The::statusBar()->shortMessage( i18n( "Error starting track from Last.Fm radio" )   );
            }
        }


    private:
        KUrl m_url;
        LastFm::TrackPtr m_track;

        
        lastfm::Track m_currentTrack;
        lastfm::RadioTuner* m_tuner;
};

#endif
