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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef MTPWRITECAPABILITY_H
#define MTPWRITECAPABILITY_H

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
    explicit MtpWriteCapability( Meta::MtpHandler *handler );

    QStringList supportedFormats() override;

    void findPathToCopy( const Meta::TrackPtr &srcTrack, const Meta::MediaDeviceTrackPtr &destTrack ) override;

    bool libCopyTrack( const Meta::TrackPtr &srcTrack, Meta::MediaDeviceTrackPtr &destTrack ) override;

    bool libDeleteTrackFile( const Meta::MediaDeviceTrackPtr &track ) override;

    void libCreateTrack( const Meta::MediaDeviceTrackPtr &track ) override;

    void libDeleteTrack( const Meta::MediaDeviceTrackPtr &track ) override;

    void addTrackInDB( const Meta::MediaDeviceTrackPtr &track ) override;

    void removeTrackFromDB( const Meta::MediaDeviceTrackPtr &track ) override;

    void setDatabaseChanged() override;

    void libSetTitle( Meta::MediaDeviceTrackPtr &track, const QString& title ) override;
    void libSetAlbum( Meta::MediaDeviceTrackPtr &track, const QString& album ) override;
    void libSetArtist( Meta::MediaDeviceTrackPtr &track, const QString& artist ) override;
    void libSetAlbumArtist( Meta::MediaDeviceTrackPtr &track, const QString& albumArtist ) override;
    void libSetComposer( Meta::MediaDeviceTrackPtr &track, const QString& composer ) override;
    void libSetGenre( Meta::MediaDeviceTrackPtr &track, const QString& genre ) override;
    void libSetYear( Meta::MediaDeviceTrackPtr &track, const QString& year ) override;
    void libSetLength( Meta::MediaDeviceTrackPtr &track, int length ) override;
    void libSetTrackNumber( Meta::MediaDeviceTrackPtr &track, int tracknum ) override;
    void libSetComment( Meta::MediaDeviceTrackPtr &track, const QString& comment ) override;
    void libSetDiscNumber( Meta::MediaDeviceTrackPtr &track, int discnum ) override;
    void libSetBitrate( Meta::MediaDeviceTrackPtr &track, int bitrate ) override;
    void libSetSamplerate( Meta::MediaDeviceTrackPtr &track, int samplerate ) override;
    void libSetBpm( Meta::MediaDeviceTrackPtr &track, qreal bpm ) override;
    void libSetFileSize( Meta::MediaDeviceTrackPtr &track, int filesize ) override;
    void libSetPlayCount( Meta::MediaDeviceTrackPtr &track, int playcount ) override;
    void libSetLastPlayed( Meta::MediaDeviceTrackPtr &track, const QDateTime &lastplayed ) override;
    void libSetRating( Meta::MediaDeviceTrackPtr &track, int rating )  override;
    void libSetType( Meta::MediaDeviceTrackPtr &track, const QString& type ) override;
    void libSetPlayableUrl( Meta::MediaDeviceTrackPtr &destTrack, const Meta::TrackPtr &srcTrack ) override;
    void libSetCoverArt( Meta::MediaDeviceTrackPtr &track, const QImage &cover ) override;

    void prepareToCopy() override;
    void prepareToDelete() override;

    void updateTrack( Meta::MediaDeviceTrackPtr &track ) override;

    private:
        Meta::MtpHandler *m_handler;
};

}

#endif
