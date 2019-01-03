/****************************************************************************************
 * Copyright (c) 2009 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
 * Copyright (c) 2009 Mark Kretschmann <kretschmann@kde.org>                            *
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

#ifndef MTPREADCAPABILITY_H
#define MTPREADCAPABILITY_H

#include "mediadevicecollection_export.h"
#include "ReadCapability.h"

#include <QPointer>


namespace Meta {
    class MtpHandler;
}

namespace Handler
{

class MtpReadCapability : public ReadCapability
{
    Q_OBJECT

    public:
        explicit MtpReadCapability( Meta::MtpHandler *handler );

        void prepareToParseTracks() override;

        bool isEndOfParseTracksList() override;

        void prepareToParseNextTrack() override;

        void nextTrackToParse() override;

        void setAssociateTrack( const Meta::MediaDeviceTrackPtr track ) override;

        QString libGetTitle( const Meta::MediaDeviceTrackPtr &track ) override;
        QString libGetAlbum( const Meta::MediaDeviceTrackPtr &track ) override;
        QString libGetArtist( const Meta::MediaDeviceTrackPtr &track ) override;
        QString libGetAlbumArtist( const Meta::MediaDeviceTrackPtr &track ) override;
        QString libGetComposer( const Meta::MediaDeviceTrackPtr &track ) override;
        QString libGetGenre( const Meta::MediaDeviceTrackPtr &track ) override;
        int     libGetYear( const Meta::MediaDeviceTrackPtr &track ) override;
        qint64  libGetLength( const Meta::MediaDeviceTrackPtr &track ) override;
        int     libGetTrackNumber( const Meta::MediaDeviceTrackPtr &track ) override;
        QString libGetComment( const Meta::MediaDeviceTrackPtr &track ) override;
        int     libGetDiscNumber( const Meta::MediaDeviceTrackPtr &track ) override;
        int     libGetBitrate( const Meta::MediaDeviceTrackPtr &track ) override;
        int     libGetSamplerate( const Meta::MediaDeviceTrackPtr &track ) override;
        qreal   libGetBpm( const Meta::MediaDeviceTrackPtr &track ) override;
        int     libGetFileSize( const Meta::MediaDeviceTrackPtr &track ) override;
        int     libGetPlayCount( const Meta::MediaDeviceTrackPtr &track ) override;
        QDateTime libGetLastPlayed( const Meta::MediaDeviceTrackPtr &track ) override;
        int     libGetRating( const Meta::MediaDeviceTrackPtr &track ) override;
        QString libGetType( const Meta::MediaDeviceTrackPtr &track ) override;
        QUrl libGetPlayableUrl( const Meta::MediaDeviceTrackPtr &track ) override;

        float usedCapacity() const override;
        float totalCapacity() const override;

    private:
        QPointer<Meta::MtpHandler> m_handler;
};

}

#endif
