/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

//
#include "ScriptableServiceInfoParser.h"
#include "../ServiceMetaBase.h"
#include "dialogs/ScriptManager.h"

using namespace Meta;

ScriptableServiceInfoParser::ScriptableServiceInfoParser( const QString &serviceName )
 : InfoParserBase()
 , m_serviceName( serviceName )
{
}


ScriptableServiceInfoParser::~ScriptableServiceInfoParser()
{
}

void ScriptableServiceInfoParser::getInfo( ArtistPtr artist )
{
    ScriptableServiceArtist * serviceArtist = dynamic_cast< ScriptableServiceArtist * >( artist.data() );
    if (serviceArtist == 0) return;

    emit( info( serviceArtist->description() ) );

    if ( serviceArtist->description().isEmpty() )
    {
        showLoading( i18n( "Loading info..." ) );
        ScriptManager::instance()->ServiceScriptRequestInfo( m_serviceName, serviceArtist->level(), serviceArtist->callbackString() );
    }

    //FIXME: when out of string freeze, add a "fething info" comment instead of just useing blank info

}

void ScriptableServiceInfoParser::getInfo(AlbumPtr album)
{
    DEBUG_BLOCK
    ScriptableServiceAlbum * serviceAlbum = dynamic_cast< ScriptableServiceAlbum * >( album.data() );
    if (serviceAlbum == 0) return;

    emit( info( serviceAlbum->description() ) );

    if ( serviceAlbum->description().isEmpty() )
    {
        showLoading( i18n( "Loading info..." ) );
        ScriptManager::instance()->ServiceScriptRequestInfo( m_serviceName, serviceAlbum->level(), serviceAlbum->callbackString() );
    }

}

void ScriptableServiceInfoParser::getInfo(TrackPtr track)
{
    DEBUG_BLOCK
    ScriptableServiceTrack * serviceTrack = dynamic_cast< ScriptableServiceTrack * >( track.data() );
    if (serviceTrack == 0) return;

    emit( info( serviceTrack->description() ) );

    if ( serviceTrack->description().isEmpty() )
    {
        showLoading( i18n( "Loading info..." ) );
        ScriptManager::instance()->ServiceScriptRequestInfo( m_serviceName, serviceTrack->level(), serviceTrack->callbackString() );
    }

}


void ScriptableServiceInfoParser::getInfo( Meta::GenrePtr genre )
{
    ScriptableServiceGenre * serviceGenre = dynamic_cast< ScriptableServiceGenre * >( genre.data() );
    if (serviceGenre == 0) return;

    emit( info( serviceGenre->description() ) );

    if ( serviceGenre->description().isEmpty() )
    {
        showLoading( i18n( "Loading info..." ) );
        ScriptManager::instance()->ServiceScriptRequestInfo( m_serviceName, serviceGenre->level(), serviceGenre->callbackString() );
    }

}

#include "ScriptableServiceInfoParser.moc"

                 
