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

#ifndef MTPHANDLER_WRITECAPABILITY_H
#define MTPHANDLER_WRITECAPABILITY_H

#include "mediadevicecollection_export.h"
#include "WriteCapability.h"

namespace Meta {
    class MtpHandler;
}

namespace Handler
{

class MtpWriteCapability : public WriteCapability
{
    Q_OBJECT
    public:
    MtpWriteCapability( Meta::MtpHandler *handler );

    virtual QStringList supportedFormats();

    virtual void findPathToCopy( const Meta::TrackPtr &srcTrack, const Meta::MediaDeviceTrackPtr &destTrack );

    virtual bool libCopyTrack( const Meta::TrackPtr &srcTrack, Meta::MediaDeviceTrackPtr &destTrack );

    virtual bool libDeleteTrackFile( const Meta::MediaDeviceTrackPtr &track );

    virtual void libCreateTrack( const Meta::MediaDeviceTrackPtr &track );

    virtual void libDeleteTrack( const Meta::MediaDeviceTrackPtr &track );

    virtual void addTrackInDB( const Meta::MediaDeviceTrackPtr &track );

    virtual void removeTrackFromDB( const Meta::MediaDeviceTrackPtr &track );

    virtual void databaseChanged();

    virtual void libSetTitle( Meta::MediaDeviceTrackPtr &track, const QString& title );
    virtual void libSetAlbum( Meta::MediaDeviceTrackPtr &track, const QString& album );
    virtual void libSetArtist( Meta::MediaDeviceTrackPtr &track, const QString& artist );
    virtual void libSetComposer( Meta::MediaDeviceTrackPtr &track, const QString& composer );
    virtual void libSetGenre( Meta::MediaDeviceTrackPtr &track, const QString& genre );
    virtual void libSetYear( Meta::MediaDeviceTrackPtr &track, const QString& year );
    virtual void libSetLength( Meta::MediaDeviceTrackPtr &track, int length );
    virtual void libSetTrackNumber( Meta::MediaDeviceTrackPtr &track, int tracknum );
    virtual void libSetComment( Meta::MediaDeviceTrackPtr &track, const QString& comment );
    virtual void libSetDiscNumber( Meta::MediaDeviceTrackPtr &track, int discnum );
    virtual void libSetBitrate( Meta::MediaDeviceTrackPtr &track, int bitrate );
    virtual void libSetSamplerate( Meta::MediaDeviceTrackPtr &track, int samplerate );
    virtual void libSetBpm( Meta::MediaDeviceTrackPtr &track, float bpm );
    virtual void libSetFileSize( Meta::MediaDeviceTrackPtr &track, int filesize );
    virtual void libSetPlayCount( Meta::MediaDeviceTrackPtr &track, int playcount );
    virtual void libSetLastPlayed( Meta::MediaDeviceTrackPtr &track, uint lastplayed );
    virtual void libSetRating( Meta::MediaDeviceTrackPtr &track, int rating ) ;
    virtual void libSetType( Meta::MediaDeviceTrackPtr &track, const QString& type );
    virtual void libSetPlayableUrl( Meta::MediaDeviceTrackPtr &destTrack, const Meta::TrackPtr &srcTrack );

    virtual void prepareToCopy();
    virtual void prepareToDelete();

    virtual void updateTrack( Meta::MediaDeviceTrackPtr &track );

    private:
        Meta::MtpHandler *m_handler;
};

}

#endif
