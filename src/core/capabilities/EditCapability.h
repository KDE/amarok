/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef AMAROK_EDITCAPABILITY_H
#define AMAROK_EDITCAPABILITY_H

#include "shared/amarok_export.h"
#include "core/capabilities/Capability.h"

namespace Capabilities
{
    class AMAROK_CORE_EXPORT EditCapability : public Capabilities::Capability
    {
        Q_OBJECT
        public:
            virtual ~EditCapability();

            static Type capabilityInterfaceType() { return Capabilities::Capability::Editable; }

            /** Returns true if the tags of this track are currently editable */
            virtual bool isEditable() const = 0;
            /** Update the album of this track. */
            virtual void setAlbum( const QString &newAlbum ) = 0;
            //TODO: add overloaded methods which take a AlbumPtr if necessary
            /** Change the artist of this track */
            virtual void setArtist( const QString &newArtist ) = 0;

            virtual void setComposer( const QString &newComposer ) = 0;

            virtual void setGenre( const QString &newGenre ) = 0;

            virtual void setYear( const QString &newYear ) = 0;

            virtual void setTitle( const QString &newTitle ) = 0;

            virtual void setComment( const QString &newComment ) = 0;

            virtual void setTrackNumber( int newTrackNumber ) = 0;

            virtual void setDiscNumber( int newDiscNumber ) = 0;

            virtual void setBpm( const qreal newBpm ) = 0;

            /** The track object should not store changed meta data immediately but cache the
            changes until endMetaDataUpdate() or abortMetaDataUpdate() is called */
            virtual void beginMetaDataUpdate() = 0;
            /** All meta data has been updated and the object should commit the changed */
            virtual void endMetaDataUpdate() = 0;
            /** Abort the meta data update without committing the changes */
            virtual void abortMetaDataUpdate() = 0;

    };
}

#endif
