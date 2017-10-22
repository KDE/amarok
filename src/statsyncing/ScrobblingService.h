/****************************************************************************************
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

#ifndef STATSYNCING_SCROBBLINGSERVICE_H
#define STATSYNCING_SCROBBLINGSERVICE_H

#include "amarok_export.h"
#include "support/QSharedDataPointerMisc.h" // operator<() for ScrobblingServicePtr

#include <QDateTime>
#include <QMetaType>
#include <QSharedPointer>

template<class T> class KSharedPtr;
namespace Meta {
    class Track;
    typedef KSharedPtr<Track> TrackPtr;
}

namespace StatSyncing
{
    /**
     * Abstract base class for classes that provide track play scrobbling to Last.fm or
     * some similar service.
     *
     * This class is memory-managed as explicitly shared data, use ScrobblingServicePtr
     * every time you store reference to this class.
     */
    // virtual inheritance to fight dreaded diamond problem in last.fm class
    // http://www.parashift.com/c++-faq-lite/mi-diamond.html
    class AMAROK_EXPORT ScrobblingService
    {
        public:
            virtual ~ScrobblingService();

            enum ScrobbleError {
                NoError,
                TooShort, // to short song or too short played time
                BadMetadata, // invalid artist, album, title...
                FromTheFuture,
                FromTheDistantPast,
                SkippedByUser //track contains label to be skipped by user in lastfm config
            };

            /**
             * Return (possibly) localized user-displayable pretty name identifying
             * this scrobbling service.
             */
            virtual QString prettyName() const = 0;

            /**
             * Scrobble a track. Scrobbling service may check certain criteria such as
             * track length and refuse to scrobble the track.
             *
             * @param track track to scrobble; you may assume it is non-null
             * @param playedFraction fraction which has been actually played, or a number
             *                       greater than 1 if the track was played multiple times
             *                       (for example on a media device)
             * @param time time when it was played, invalid QDateTime signifies that the
             *             track has been played just now. This is the default when the
             *             parameter is omitted.
             */
            virtual ScrobbleError scrobble( const Meta::TrackPtr &track, double playedFraction = 1.0,
                                            const QDateTime &time = QDateTime() ) = 0;

            /**
             * Update the "Now Playing" info on the scrobbling site without scrobbling the
             * track permanently. Scrobbler may check certain criteria and refuse to update
             * Now Playing if they are not met. If track is null, it means that no track is
             * playing and scrobbler shoudl clear the Now Playing status. You may safely
             * assume this is not called too frequently.
             *
             * @param track that is currently playing or null if playbak was stopped
             */
            virtual void updateNowPlaying( const Meta::TrackPtr &track ) = 0;
    };

    typedef QSharedPointer<ScrobblingService> ScrobblingServicePtr;
}

Q_DECLARE_METATYPE( StatSyncing::ScrobblingServicePtr )

#endif // STATSYNCING_SCROBBLINGSERVICE_H
