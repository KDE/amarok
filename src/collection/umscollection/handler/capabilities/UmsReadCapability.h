/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
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

#ifndef UMSREADCAPABILITY_H
#define UMSREADCAPABILITY_H

#include "CustomReadCapability.h"

namespace Meta {
    class UmsHandler;
}

namespace Handler
{

class UmsReadCapability : public CustomReadCapability
{
        Q_OBJECT

    public:
        UmsReadCapability( Meta::UmsHandler *handler );

        virtual void prepareToParseTracks();

        virtual bool isEndOfParseTracksList();

        virtual void prepareToParseNextTrack();

        virtual void nextTrackToParse();

        virtual void setAssociateTrack( const Meta::MediaDeviceTrackPtr track );

        virtual float usedCapacity() const;
        virtual float totalCapacity() const;

        virtual QString libGetAlbum( const Meta::MediaDeviceTrackPtr &track );
        virtual QString libGetArtist( const Meta::MediaDeviceTrackPtr &track );
        virtual QString libGetComposer( const Meta::MediaDeviceTrackPtr &track );
        virtual QString libGetGenre( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetYear( const Meta::MediaDeviceTrackPtr &track );

        virtual Meta::TrackPtr sourceTrack();

    private:
        Meta::UmsHandler *m_handler;
};

}

#endif
