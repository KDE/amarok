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
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef UMSHANDLER_WRITECAPABILITY_H
#define UMSHANDLER_WRITECAPABILITY_H

#include "WriteCapabilityBase.h"

namespace Meta {
    class UmsHandler;
}

namespace Handler
{

class UmsWriteCapability : public WriteCapabilityBase
{
    Q_OBJECT
    public:
    UmsWriteCapability( Meta::UmsHandler *handler );

    virtual QStringList supportedFormats();

    virtual void findPathToCopy( const Meta::TrackPtr &srcTrack, const Meta::MediaDeviceTrackPtr &destTrack );

    virtual bool libCopyTrack( const Meta::TrackPtr &srcTrack, Meta::MediaDeviceTrackPtr &destTrack );

    virtual bool libDeleteTrackFile( const Meta::MediaDeviceTrackPtr &track );

    virtual void libSetPlayableUrl( Meta::MediaDeviceTrackPtr &destTrack, const Meta::TrackPtr &srcTrack );

    virtual void prepareToCopy();
    virtual void prepareToDelete();

    virtual void updateTrack( Meta::MediaDeviceTrackPtr &track );

    virtual void endTrackRemove();

    private:
        Meta::UmsHandler *m_handler;
};

}

#endif
