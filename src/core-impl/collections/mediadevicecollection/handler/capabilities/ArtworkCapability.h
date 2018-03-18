/****************************************************************************************
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
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

#ifndef MEDIADEVICEHANDLER_ARTWORK_CAPABILITY_H
#define MEDIADEVICEHANDLER_ARTWORK_CAPABILITY_H

#include "core-impl/collections/mediadevicecollection/MediaDeviceMeta.h"
#include "core-impl/collections/mediadevicecollection/handler/MediaDeviceHandlerCapability.h"

namespace Handler
{
    class MEDIADEVICECOLLECTION_EXPORT ArtworkCapability : public Handler::Capability
    {
        Q_OBJECT

        public:
            explicit ArtworkCapability( QObject *parent ) : Capability( parent ) {}
            virtual ~ArtworkCapability();

            virtual QImage getCover( const Meta::MediaDeviceTrackPtr &track ) = 0;

            virtual void setCover( Meta::MediaDeviceAlbumPtr album, const QImage &image ) = 0;

            virtual void setCoverPath( Meta::MediaDeviceAlbumPtr album, const QString &path );

            virtual bool canUpdateCover() const = 0;

            static Type capabilityInterfaceType() { return Handler::Capability::Artwork; }
    };
}

#endif
