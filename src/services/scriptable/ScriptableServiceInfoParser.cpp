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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

//
#include "ScriptableServiceInfoParser.h"
#include "../ServiceMetaBase.h"

using namespace Meta;

ScriptableServiceInfoParser::ScriptableServiceInfoParser()
 : InfoParserBase()
{
}


ScriptableServiceInfoParser::~ScriptableServiceInfoParser()
{
}

void ScriptableServiceInfoParser::getInfo(ArtistPtr artist)
{
    ServiceArtist * serviceArtist = dynamic_cast< ServiceArtist * >( artist.data() );
    if (serviceArtist == 0) return;
    emit( info( serviceArtist->description() ) );
}

void ScriptableServiceInfoParser::getInfo(AlbumPtr album)
{
    DEBUG_BLOCK
    ServiceAlbum * serviceAlbum = dynamic_cast< ServiceAlbum * >( album.data() );
    if (serviceAlbum == 0) return;
    emit( info( serviceAlbum->description() ) );
}

void ScriptableServiceInfoParser::getInfo(TrackPtr track)
{
    DEBUG_BLOCK
    ServiceTrack * serviceTrack = dynamic_cast< ServiceTrack * >( track.data() );
    if (serviceTrack == 0) return;
    emit( info( serviceTrack->sourceDescription() ) );
}


void ScriptableServiceInfoParser::getInfo(Meta::GenrePtr genre)
{
    ScriptableServiceGenre * serviceGenre = dynamic_cast< ScriptableServiceGenre * >( genre.data() );
    if (serviceGenre == 0) return;
    emit( info( serviceGenre->description() ) );
}

#include "ScriptableServiceInfoParser.moc"

                 
