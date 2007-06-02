/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#include "JamendoInfoParser.h"
#include "servicemetabase.h"

JamendoInfoParser::JamendoInfoParser()
 : InfoParserBase()
{
}


JamendoInfoParser::~JamendoInfoParser()
{
}

void JamendoInfoParser::getInfo(ArtistPtr artist)
{

    ServiceArtist * serviceArtist = dynamic_cast<ServiceArtist *> ( artist.data() );
    if ( serviceArtist == 0) return;

    QString infoHtml = "<HTML><HEAD><META HTTP-EQUIV=\"Content-Type\" "
                       "CONTENT=\"text/html; charset=iso-8859-1\"></HEAD><BODY>";
    infoHtml +=        "<div align=\"center\">";
    infoHtml +=        "Artist<br><br>";
    infoHtml +=        "<strong>";
    infoHtml +=        serviceArtist->fullPrettyName();
    infoHtml +=        "</strong><br><br><em>";
    infoHtml +=        "<br><br>From Jamendo.com</div>";
    infoHtml +=        "</BODY></HTML>";

    emit( info( infoHtml ) );
}

void JamendoInfoParser::getInfo(AlbumPtr album)
{
    ServiceAlbum * serviceAlbum = dynamic_cast<ServiceAlbum *> ( album.data() );
    if ( serviceAlbum == 0) return;

    QString infoHtml = "<HTML><HEAD><META HTTP-EQUIV=\"Content-Type\" "
                       "CONTENT=\"text/html; charset=iso-8859-1\"></HEAD><BODY>";
    infoHtml +=        "<div align=\"center\">";
    infoHtml +=        "Album<br><br>";
    infoHtml +=        "<strong>";
    infoHtml +=        serviceAlbum->fullPrettyName();
    infoHtml +=        "</strong><br><br><em>";
    infoHtml +=        "<br><br>From Jamendo.com</div>";
    infoHtml +=        "</BODY></HTML>";

    emit( info( infoHtml ) );
}

void JamendoInfoParser::getInfo(TrackPtr track)
{
    ServiceTrack * serviceTrack = dynamic_cast<ServiceTrack *> ( track.data() );
    if ( serviceTrack == 0) return;

    QString infoHtml = "<HTML><HEAD><META HTTP-EQUIV=\"Content-Type\" "
                       "CONTENT=\"text/html; charset=iso-8859-1\"></HEAD><BODY>";
    infoHtml +=        "<div align=\"center\">";
    infoHtml +=        "Track<br><br>";
    infoHtml +=        "<strong>";
    infoHtml +=        serviceTrack->fullPrettyName();
    infoHtml +=        "</strong><br><br><em>";
    infoHtml +=        "<br><br>From Jamendo.com</div>";
    infoHtml +=        "</BODY></HTML>";

    emit( info( infoHtml ) );
}

#include "JamendoInfoParser.moc"
