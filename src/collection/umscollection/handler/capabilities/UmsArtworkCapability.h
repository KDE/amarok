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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef IPODHANDLER_ARTWORK_CAPABILITY_H
#define IPODHANDLER_ARTWORK_CAPABILITY_H

#include "ArtworkCapability.h"
#include "../../MediaDeviceMeta.h"

namespace Meta {
    class IpodHandler;
}

namespace Handler
{
    class IpodArtworkCapability : public ArtworkCapability
    {
        Q_OBJECT

        public:
            IpodArtworkCapability( Meta::IpodHandler *handler );
            virtual ~IpodArtworkCapability();

            virtual QPixmap getCover( const Meta::MediaDeviceTrackPtr &track );

            virtual bool canUpdateCover() const;

            static Type capabilityInterfaceType() { return Handler::Capability::Artwork; }

        private:
            Meta::IpodHandler *m_handler;
    };
}

#endif
