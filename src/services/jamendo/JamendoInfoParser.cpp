/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "JamendoInfoParser.h"

#include "core/support/Debug.h"
#include "JamendoMeta.h"

#include <KLocalizedString>

using namespace Meta;

JamendoInfoParser::JamendoInfoParser()
 : InfoParserBase()
{
}

JamendoInfoParser::~JamendoInfoParser()
{
}

void
JamendoInfoParser::getInfo(const ArtistPtr &artist)
{
    DEBUG_BLOCK
    JamendoArtist * jamendoArtist = dynamic_cast<JamendoArtist *> ( artist.data() );
    if ( jamendoArtist == 0)
        return;

    QString description = jamendoArtist->description();

    if ( description.isEmpty() )
        description = i18n( "No description available..." );

    QString infoHtml = "<HTML><HEAD><META HTTP-EQUIV=\"Content-Type\" "
                       "CONTENT=\"text/html; charset=utf-8\"></HEAD><BODY>";
    infoHtml +=        "<div align=\"center\">";
    infoHtml +=        i18n( "Artist" ) + "<br><br>";
    infoHtml +=        "<strong>";
    infoHtml +=        jamendoArtist->prettyName();
    infoHtml +=        "</strong><br><br><em>";

    if ( !jamendoArtist->photoURL().isEmpty() )
        infoHtml +=    "<img src=\"" + jamendoArtist->photoURL() +
                       "\" align=\"middle\" border=\"1\"><br><br>";

    infoHtml +=        description;
    infoHtml +=        "<br><br>" + i18n( "From Jamendo.com" ) + "</div>";
    infoHtml +=        "</BODY></HTML>";

    emit( info( infoHtml ) );
}

void
JamendoInfoParser::getInfo(const AlbumPtr &album)
{
    DEBUG_BLOCK
    JamendoAlbum * jamendoAlbum = dynamic_cast<JamendoAlbum *> ( album.data() );
    if ( jamendoAlbum == 0) return;

    QString description = jamendoAlbum->description();

    if ( description.isEmpty() )
        description = i18n( "No description available..." );


    QString infoHtml = "<HTML><HEAD><META HTTP-EQUIV=\"Content-Type\" "
                       "CONTENT=\"text/html; charset=utf-8\"></HEAD><BODY>";
    infoHtml +=        "<div align=\"center\">";
    infoHtml +=        i18n( "Album" ) + "<br><br>";
    infoHtml +=        "<strong>";
    infoHtml +=        jamendoAlbum->prettyName();
    infoHtml +=        "</strong><br><br><em>";

    if ( !jamendoAlbum->coverUrl().isEmpty() )
        infoHtml +=    "<img src=\"" + jamendoAlbum->coverUrl() +
                       "\" align=\"middle\" border=\"1\"><br><br>";

    infoHtml +=        description;
    infoHtml +=        "<br><br>" + i18n( "From Jamendo.com" ) + "</div>";
    infoHtml +=        "</BODY></HTML>";

    emit( info( infoHtml ) );
}

void
JamendoInfoParser::getInfo(const TrackPtr &track)
{
    DEBUG_BLOCK
    JamendoTrack * jamendoTrack = dynamic_cast<JamendoTrack *> ( track.data() );
    if ( jamendoTrack == 0) return;

    QString infoHtml = "<HTML><HEAD><META HTTP-EQUIV=\"Content-Type\" "
                       "CONTENT=\"text/html; charset=utf-8\"></HEAD><BODY>";
    infoHtml +=        "<div align=\"center\">";
    infoHtml +=        i18n( "Track" ) + "<br><br>";
    infoHtml +=        "<strong>";
    infoHtml +=        jamendoTrack->prettyName();
    infoHtml +=        "</strong><br><br><em>";
    infoHtml +=        "<br><br>" + i18n( "From Jamendo.com" ) + "</div>";
    infoHtml +=        "</BODY></HTML>";

    emit( info( infoHtml ) );
}


