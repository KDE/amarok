/****************************************************************************************
 * Copyright (c) 2013 Anmol Ahuja <darthcodus@gmail.com>                                *
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

#include "PlaylistProviderExporter.h"

#include "playlistmanager/PlaylistManager.h"
#include "core/playlists/PlaylistProvider.h"
#include "core-impl/playlists/providers/user/UserPlaylistProvider.h"
#include "scripting/scriptengine/ScriptingDefines.h"

#include <KIcon>

#include <QScriptEngine>
#include <QScriptValue>

using namespace AmarokScript;

PlaylistProviderPrototype::PlaylistProviderPrototype( Playlists::PlaylistProvider *provider )
: QObject( 0 )
, m_provider( provider )
{}

void
PlaylistProviderPrototype::init( QScriptEngine *engine )
{
    qScriptRegisterMetaType<Playlists::PlaylistProvider*>( engine, toScriptValue<Playlists::PlaylistProvider*,PlaylistProviderPrototype>,
                                                           fromScriptValue<Playlists::PlaylistProvider*,PlaylistProviderPrototype> );
    qScriptRegisterMetaType< PlaylistProviderList >( engine, toScriptArray, fromScriptArray );
}

// script invokable

Playlists::PlaylistPtr
PlaylistProviderPrototype::addPlaylist(Playlists::PlaylistPtr playlist)
{
    if( m_provider )
        return m_provider.data()->addPlaylist( playlist );
    return Playlists::PlaylistPtr();
}

int
PlaylistProviderPrototype::category() const
{
    if( m_provider )
        return m_provider.data()->category();
    return -1;
}

bool
PlaylistProviderPrototype::deletePlaylists( const Playlists::PlaylistList &playlistlist )
{
    return m_provider && m_provider.data()->deletePlaylists( playlistlist );
}

Playlists::PlaylistList
PlaylistProviderPrototype::playlists()
{
    if( m_provider )
        return m_provider.data()->playlists();
    return Playlists::PlaylistList();
}

void
PlaylistProviderPrototype::renamePlaylist( Playlists::PlaylistPtr playlist, const QString &newName )
{
    if( m_provider )
        m_provider.data()->renamePlaylist( playlist, newName );
}

Playlists::PlaylistPtr
PlaylistProviderPrototype::save( const Meta::TrackList &tracks, const QString &name )
{
    Playlists::UserPlaylistProvider* playlist = dynamic_cast<Playlists::UserPlaylistProvider*>( m_provider.data() );
    if( playlist )
        return playlist->save( tracks, name );
    return Playlists::PlaylistPtr();
}

QString PlaylistProviderPrototype::toString() const
{
    if( m_provider )
        return m_provider.data()->prettyName();
    return QString( "Invalid" );
}

// private

bool
PlaylistProviderPrototype::isValid() const
{
    return m_provider;
}

QIcon
PlaylistProviderPrototype::icon() const
{
    if( m_provider )
        return m_provider.data()->icon();
    return QIcon();
}

bool
PlaylistProviderPrototype::isWritable() const
{
    return m_provider && m_provider.data()->isWritable();
}

int
PlaylistProviderPrototype::playlistCount() const
{
    if( m_provider )
        return m_provider.data()->playlistCount();
    return -1;
}
