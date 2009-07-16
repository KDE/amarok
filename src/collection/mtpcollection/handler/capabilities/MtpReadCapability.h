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

#ifndef MTPHANDLER_READCAPABILITY_H
#define MTPHANDLER_READCAPABILITY_H

#include "mediadevicecollection_export.h"
#include "ReadCapability.h"

namespace Meta {
    class MtpHandler;
}

namespace Handler
{

class MtpReadCapability : public ReadCapability
{
    Q_OBJECT
    public:
        MtpReadCapability( Meta::MtpHandler *handler );

        virtual void prepareToParseTracks();

        virtual bool isEndOfParseTracksList();

        virtual void prepareToParseNextTrack();

        virtual void nextTrackToParse();

        virtual void setAssociateTrack( const Meta::MediaDeviceTrackPtr track );

        virtual QString libGetTitle( const Meta::MediaDeviceTrackPtr &track );
        virtual QString libGetAlbum( const Meta::MediaDeviceTrackPtr &track );
        virtual QString libGetArtist( const Meta::MediaDeviceTrackPtr &track );
        virtual QString libGetComposer( const Meta::MediaDeviceTrackPtr &track );
        virtual QString libGetGenre( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetYear( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetLength( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetTrackNumber( const Meta::MediaDeviceTrackPtr &track );
        virtual QString libGetComment( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetDiscNumber( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetBitrate( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetSamplerate( const Meta::MediaDeviceTrackPtr &track );
        virtual float   libGetBpm( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetFileSize( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetPlayCount( const Meta::MediaDeviceTrackPtr &track );
        virtual uint    libGetLastPlayed( const Meta::MediaDeviceTrackPtr &track );
        virtual int     libGetRating( const Meta::MediaDeviceTrackPtr &track );
        virtual QString libGetType( const Meta::MediaDeviceTrackPtr &track );
        virtual KUrl libGetPlayableUrl( const Meta::MediaDeviceTrackPtr &track );

        virtual float usedCapacity() const;
        virtual float totalCapacity() const;

        private:
            Meta::MtpHandler *m_handler;
};

}

#endif
