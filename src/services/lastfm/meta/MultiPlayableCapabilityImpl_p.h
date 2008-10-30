/*
   Copyright (C) 2008 Shane King <kde@dontletsstart.com>

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

#ifndef AMAROK_MULTIPLAYABLECAPABILITYIMPL_P_H
#define AMAROK_MULTIPLAYABLECAPABILITYIMPL_P_H

#include "Debug.h"
#include "LastFmServiceConfig.h"
#include "Meta.h"
#include "meta/MultiPlayableCapability.h"

#include <lastfm/types/Track.h>
#include <lastfm/radio/Tuner.h>

#include <QQueue>
#include <QCryptographicHash>

class MultiPlayableCapabilityImpl : public Meta::MultiPlayableCapability, public Meta::Observer
{
    Q_OBJECT
    public:
        MultiPlayableCapabilityImpl( LastFm::Track *track )
            : Meta::MultiPlayableCapability()
            , m_url( track->internalUrl() )
            , m_track( track )
            , m_currentTrack( Track() )
        {
            Meta::TrackPtr trackptr( track );
            subscribeTo( trackptr );
            
            connect( track, SIGNAL( skipTrack() ), this, SLOT( skip() ) );
        }

        virtual ~MultiPlayableCapabilityImpl() 
        {}

        virtual void fetchFirst()
        {
            DEBUG_BLOCK
            // first play, so we need to fetch tracks and start playing
            // tuner needs md5 hashed password...
            LastFmServiceConfig config;
            QString pw = config.password();
            
            QByteArray const digest = QCryptographicHash::hash( pw.toUtf8(), QCryptographicHash::Md5 );
            pw = QString::fromLatin1( digest.toHex() ).rightJustified( 32, '0' );
            
            m_tuner = new lastfm::legacy::Tuner( RadioStation( m_track->uidUrl() ), pw );
            
            connect( m_tuner, SIGNAL( tracks( const QList< Track >& ) ), this, SLOT( slotNewTracks( const QList< Track >& ) ) );

            //m_tuner->fetchFiveMoreTracks();
        }
        
        virtual void fetchNext()
        {
            DEBUG_BLOCK
            if( m_upcomingTracks.size() == 0 ) // out of tracks, stop
            {
                debug() << "OUT OF TRACKS, STOP ME HERE";
                return;
            } else if( m_upcomingTracks.size() == 1 ) // fetch more after we start playing
            {
                m_currentTrack = m_upcomingTracks.dequeue();
                debug() << "getting more tracks, first playing this: " << m_currentTrack;
                m_track->setTrackInfo( m_currentTrack );
                m_tuner->fetchFiveMoreTracks();
            } else
            {
                m_currentTrack = m_upcomingTracks.dequeue();
                debug() << "i have" << m_upcomingTracks.size() << "more stored tracks, next up: " << m_currentTrack;
                m_track->setTrackInfo( m_currentTrack );
            }
        }
        
        using Observer::metadataChanged;
        virtual void metadataChanged( Meta::TrackPtr track )
        {
            DEBUG_BLOCK
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
        
        void slotNewTracks( const QList< Track >& tracks )
        {
            DEBUG_BLOCK
            foreach( Track track,  tracks )
                m_upcomingTracks.enqueue( track );

            if( m_currentTrack.isNull() ) // start playing
            {
                m_currentTrack = m_upcomingTracks.dequeue();
                debug() << "first track starting:" << m_currentTrack.url();
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


    private:
        KUrl m_url;
        LastFm::TrackPtr m_track;

        
        Track m_currentTrack;
        QQueue< Track > m_upcomingTracks;
        lastfm::legacy::Tuner* m_tuner;
};

#endif
