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

#ifndef IPODMETAEDITCAPABILITY_H
#define IPODMETAEDITCAPABILITY_H

#include "IpodMeta.h"
#include "core/meta/TrackEditor.h"

namespace IpodMeta
{
    class EditCapability : public Meta::TrackEditor
    {
        Q_OBJECT

        public:
            EditCapability( const KSharedPtr<Track> &track );
            virtual ~EditCapability();

            virtual void setAlbum( const QString &newAlbum );
            virtual void setAlbumArtist( const QString &newAlbumArtist );
            virtual void setArtist( const QString &newArtist );
            virtual void setComposer( const QString &newComposer );
            virtual void setGenre( const QString &newGenre );
            virtual void setYear( int newYear );
            virtual void setTitle( const QString &newTitle );
            virtual void setComment( const QString &newComment );
            virtual void setTrackNumber( int newTrackNumber );
            virtual void setDiscNumber( int newDiscNumber );
            virtual void setBpm( const qreal newBpm );

            virtual void beginMetaDataUpdate();
            virtual void endMetaDataUpdate();

        private:
            KSharedPtr<Track> m_track;
    };

} // namespace IpodMeta

#endif // IPODMETAEDITCAPABILITY_H
