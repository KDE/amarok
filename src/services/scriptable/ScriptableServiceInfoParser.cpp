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

#include "ScriptableServiceInfoParser.h"

#include "scripting/scriptmanager/ScriptManager.h"
#include "services/ServiceMetaBase.h"

using namespace Meta;

ScriptableServiceInfoParser::ScriptableServiceInfoParser( const QString &serviceName )
 : InfoParserBase()
 , m_serviceName( serviceName )
{
}


ScriptableServiceInfoParser::~ScriptableServiceInfoParser()
{
}

void ScriptableServiceInfoParser::getInfo( const ArtistPtr &artist )
{
    ScriptableServiceArtist * serviceArtist = dynamic_cast< ScriptableServiceArtist * >( artist.data() );
    if (!serviceArtist) return;

    Q_EMIT( info( serviceArtist->description() ) );

    if ( serviceArtist->description().isEmpty() )
    {
        showLoading( i18n( "Loading info..." ) );
        ScriptManager::instance()->ServiceScriptRequestInfo( m_serviceName, serviceArtist->level(), serviceArtist->callbackString() );
    }
}

void ScriptableServiceInfoParser::getInfo(const AlbumPtr &album)
{
    DEBUG_BLOCK
    ScriptableServiceAlbum * serviceAlbum = dynamic_cast< ScriptableServiceAlbum * >( album.data() );
    if (serviceAlbum == 0) return;

    Q_EMIT( info( serviceAlbum->description() ) );

    if ( serviceAlbum->description().isEmpty() )
    {
        showLoading( i18n( "Loading info..." ) );
        ScriptManager::instance()->ServiceScriptRequestInfo( m_serviceName, serviceAlbum->level(), serviceAlbum->callbackString() );
    }
}

void ScriptableServiceInfoParser::getInfo(const TrackPtr &track)
{
    DEBUG_BLOCK
    ScriptableServiceTrack * serviceTrack = dynamic_cast< ScriptableServiceTrack * >( track.data() );
    if (!serviceTrack) return;

    Q_EMIT  info( serviceTrack->description() );

    if ( serviceTrack->description().isEmpty() )
    {
        showLoading( i18n( "Loading info..." ) );
        ScriptManager::instance()->ServiceScriptRequestInfo( m_serviceName, serviceTrack->level(), serviceTrack->callbackString() );
    }
}

void ScriptableServiceInfoParser::getInfo( const Meta::GenrePtr &genre )
{
    ScriptableServiceGenre * serviceGenre = dynamic_cast< ScriptableServiceGenre * >( genre.data() );
    if (!serviceGenre) return;

    Q_EMIT info( serviceGenre->description() );

    if ( serviceGenre->description().isEmpty() )
    {
        showLoading( i18n( "Loading info..." ) );
        ScriptManager::instance()->ServiceScriptRequestInfo( m_serviceName, serviceGenre->level(), serviceGenre->callbackString() );
    }
}
