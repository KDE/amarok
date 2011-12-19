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

#ifndef MEDIADEVICE_EDIT_CAPABILITY_H
#define MEDIADEVICE_EDIT_CAPABILITY_H

#include "core/capabilities/EditCapability.h"
#include "MediaDeviceMeta.h"

namespace Meta
{

/**
 * This capability if different from other media device capabilities - it implements core
 * EditCapability and has nothing to do with Hander::Capability
 */
class EditCapabilityMediaDevice : public Capabilities::EditCapability
{
    Q_OBJECT

    public:
        EditCapabilityMediaDevice( MediaDeviceTrack *track )
            : Capabilities::EditCapability()
            , m_track( track ) {}
        virtual ~EditCapabilityMediaDevice();

        virtual bool isEditable() const { return m_track->isEditable(); }
        virtual void setAlbum( const QString &newAlbum ) { m_track->setAlbum( newAlbum ); }
        virtual void setAlbumArtist( const QString &newAlbumArtist ) { m_track->setAlbumArtist( newAlbumArtist ); }
        virtual void setArtist( const QString &newArtist ) { m_track->setArtist( newArtist ); }
        virtual void setComposer( const QString &newComposer ) { m_track->setComposer( newComposer ); }
        virtual void setGenre( const QString &newGenre ) { m_track->setGenre( newGenre ); }
        virtual void setYear( int newYear ) { m_track->setYear( newYear ); }
        virtual void setBpm( const qreal newBpm ) { m_track->setBpm( newBpm ); }
        virtual void setTitle( const QString &newTitle ) { m_track->setTitle( newTitle ); }
        virtual void setComment( const QString &newComment ) { m_track->setComment( newComment ); }
        virtual void setTrackNumber( int newTrackNumber ) { m_track->setTrackNumber( newTrackNumber ); }
        virtual void setDiscNumber( int newDiscNumber ) { m_track->setDiscNumber( newDiscNumber ); }
        virtual void setUidUrl( const QString &newUidUrl ) { m_track->setUidUrl( newUidUrl ); }
        virtual void beginMetaDataUpdate() { m_track->beginMetaDataUpdate(); }
        virtual void endMetaDataUpdate() { m_track->endMetaDataUpdate(); }

    private:

        MediaDeviceTrackPtr m_track;
};

}

#endif
