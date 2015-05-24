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

#ifndef IPODCOLLECTIONLOCATION_H
#define IPODCOLLECTIONLOCATION_H

#include "IpodCollection.h"
#include "jobs/IpodCopyTracksJob.h"

#include "core/collections/CollectionLocation.h"
#include "core/playlists/Playlist.h"

#include <QWeakPointer>


class IpodCollectionLocation : public Collections::CollectionLocation
{
    Q_OBJECT

    public:
        IpodCollectionLocation( QWeakPointer<IpodCollection> parentCollection );
        virtual ~IpodCollectionLocation();

        // CollectionLocation methods:
        virtual Collections::Collection *collection() const;
        virtual QString prettyLocation() const;
        virtual bool isWritable() const;

        virtual void copyUrlsToCollection( const QMap<Meta::TrackPtr, QUrl> &sources,
                                           const Transcoding::Configuration &configuration );
        virtual void removeUrlsFromCollection( const Meta::TrackList &sources );

        // IpodCollectionLocation specific methods:
        /**
         * Calling this causes that when the tracks are copied, they are added to iPod
         * playlist @param playlist
         */
        void setDestinationPlaylist( Playlists::PlaylistPtr destPlaylist,
                                     const QMap<Meta::TrackPtr, int> &trackPlaylistPositions );

        /**
         * This method is published so that IpodPlaylistProvider can hide removal dialog.
         */
        using Collections::CollectionLocation::setHidingRemoveConfirm;

    private slots:
        void slotCopyTrackProcessed( Meta::TrackPtr srcTrack, Meta::TrackPtr destTrack,
                                     IpodCopyTracksJob::CopiedStatus status );

    private:
        /**
         * Helper method to create a basic set of <ipod>/iPod_Contol/Music/F.. directories
         * so that copying doesn't fail for this simple reason
         */
        void ensureDirectoriesExist();

        QWeakPointer<IpodCollection> m_coll;
        QMap<Meta::TrackPtr, int> m_trackPlaylistPositions;
        Playlists::PlaylistPtr m_destPlaylist;
};

#endif // IPODCOLLECTIONLOCATION_H
