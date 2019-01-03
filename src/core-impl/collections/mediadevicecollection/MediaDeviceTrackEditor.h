/****************************************************************************************
 * Copyright (c) 2011 Matěj Laitl <matej@laitl.cz>                                      *
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

#ifndef MEDIADEVICE_TRACK_EDITOR_H
#define MEDIADEVICE_TRACK_EDITOR_H

#include "core/meta/TrackEditor.h"
#include "MediaDeviceMeta.h"

namespace Meta
{
class MediaDeviceTrackEditor : public Meta::TrackEditor
{
    public:
        explicit MediaDeviceTrackEditor( MediaDeviceTrack *track );

        void setAlbum( const QString &newAlbum ) override;
        void setAlbumArtist( const QString &newAlbumArtist ) override;
        void setArtist( const QString &newArtist ) override;
        void setComposer( const QString &newComposer ) override;
        void setGenre( const QString &newGenre ) override;
        void setYear( int newYear ) override;
        void setBpm( const qreal newBpm ) override;
        void setTitle( const QString &newTitle ) override;
        void setComment( const QString &newComment ) override;
        void setTrackNumber( int newTrackNumber ) override;
        void setDiscNumber( int newDiscNumber ) override;

        void beginUpdate() override;
        void endUpdate() override;

    private:
        /**
         * Tells the underlying track to write back changes if and only if current update
         * is not a part of a larger batch (initiated by beginUpdate())
         */
        void commitIfInNonBatchUpdate();

        bool m_inBatchUpdate;
        MediaDeviceTrackPtr m_track;
};

}

#endif
