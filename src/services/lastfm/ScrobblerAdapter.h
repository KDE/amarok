/****************************************************************************************
 * Copyright (c) 2007 Shane King <kde@dontletsstart.com>                                *
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
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

#ifndef LASTFMSCROBBLERADAPTER_H
#define LASTFMSCROBBLERADAPTER_H

#include "core/meta/Meta.h"

#include <QTimer>

#include <Audioscrobbler.h>
#include <Track.h>


class ScrobblerAdapter : public QObject
{
    Q_OBJECT

    public:
        ScrobblerAdapter( QObject *parent, const QString &clientId );
        virtual ~ScrobblerAdapter();

    public slots:
        /**
        * Scrobble a track. Scrobbler may check certain criteria such as track length and
        * refuse to scrobble the track.
        *
        * @param track track to scrobble; you may assume it is non-null
        * @param playedFraction fraction which has been actually played, between 0 and
        *                       1; the default is 1
        * @param time time when it was played, invalid QDateTime signifies that the
        *             track has been played just now. This is the default when the
        *             parameter is omitted.
        */
        void scrobble( const Meta::TrackPtr &track, double playedFraction = 1.0,
                       const QDateTime &time = QDateTime() );

        /**
        * Update the "Now Playing" info on the scrobbling site without scrobbling the
        * track permanently. Scrobbler may check certain criteria and refuse to update
        * Now Playing if they are not met. If track is null, it means that no track is
        * playing and scrobbler shoudl clear the Now Playing status.
        *
        * @param track that is currently playing or null if playbak was stopped
        */
        void updateNowPlaying( const Meta::TrackPtr &track );

        void loveTrack( const Meta::TrackPtr &track );
        void banTrack( const Meta::TrackPtr &track );

    private slots:
        void slotTrackFinishedPlaying( const Meta::TrackPtr &track, double playedFraction );
        void slotScrobblesSubmitted( const QList<lastfm::Track> &tracks );

        void slotResetLastSubmittedNowPlayingTrack();
        void slotUpdateNowPlayingWithCurrentTrack();
        void slotNowPlayingError( int code, const QString &message );

    private:
        /**
         * Copies metadata from @param from to @param to.
         */
        void copyTrackMetadata( lastfm::MutableTrack& to, const Meta::TrackPtr &from );

        /**
         * Return true if important metadata of both tracks is equal.
         */
        bool tracksVirtuallyEqual( const lastfm::Track &first, const lastfm::Track &second );

        /**
         * Announces Last.fm suggested @param track corrections to Amarok pop-up log.
         */
        void announceTrackCorrections( const lastfm::Track &track );

        lastfm::Audioscrobbler m_scrobbler;
        QTimer m_updateNowPlayingTimer;
        lastfm::MutableTrack m_lastSubmittedNowPlayingTrack;
};

#endif // LASTFMSCROBBLERADAPTER_H
