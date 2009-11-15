/***************************************************************************************
* Copyright (c) 2009 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
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


#ifndef LASTFMINFOPARSER_H
#define LASTFMINFOPARSER_H

#include "amarok_export.h"
#include "../InfoParserBase.h"

class QNetworkReply;

class AMAROK_EXPORT LastfmInfoParser : public InfoParserBase
{
    Q_OBJECT
    public:
        LastfmInfoParser() : InfoParserBase() {}
        ~LastfmInfoParser() {}
        virtual void getInfo(Meta::TrackPtr track);
        virtual void getInfo(Meta::AlbumPtr album);
        virtual void getInfo(Meta::ArtistPtr artist);

    private slots:
        void onGetTrackInfo();
        void onGetAlbumInfo();
        void onGetArtistInfo();

    private:
        QMap< QString, QNetworkReply* > m_jobs;

};

#endif // LASTFMINFOPARSER_H
