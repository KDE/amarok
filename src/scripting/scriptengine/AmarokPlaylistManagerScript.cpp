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

#include "AmarokPlaylistManagerScript.h"

#include "playlistmanager/PlaylistManager.h"

#include <QScriptEngine>

using namespace AmarokScript;

AmarokPlaylistManagerScript::AmarokPlaylistManagerScript( QScriptEngine* engine )
    : QObject( engine )
{
    QScriptValue scriptObject = engine->newQObject( this, QScriptEngine::AutoOwnership,
                                                    QScriptEngine::ExcludeSuperClassContents );
    engine->globalObject().property( "Amarok" ).setProperty( "PlaylistManager", scriptObject );
    //Exposes a whole bunch of other stuff, any other way of doing this?
    QScriptValue categoryEnum = engine->newQMetaObject( &PlaylistManager::staticMetaObject );
    scriptObject.setProperty( "PlaylistCategory", categoryEnum );

    PlaylistManager *instance =  PlaylistManager::instance();
    connect( instance, SIGNAL(categoryAdded(int)), SIGNAL(categoryAdded(int)) );
    connect( instance, SIGNAL(playlistAdded(Playlists::PlaylistPtr,int)), SIGNAL(playlistAdded(Playlists::PlaylistPtr,int)) );
    connect( instance, SIGNAL(playlistRemoved(Playlists::PlaylistPtr,int)), SIGNAL(playlistRemoved(Playlists::PlaylistPtr,int)) );
    connect( instance, SIGNAL(playlistUpdated(Playlists::PlaylistPtr,int)), SIGNAL(playlistUpdated(Playlists::PlaylistPtr,int)) );
    connect( instance, SIGNAL(providerAdded(Playlists::PlaylistProvider*,int)), SIGNAL(providerAdded(Playlists::PlaylistProvider*,int)) );
    connect( instance, SIGNAL(providerRemoved(Playlists::PlaylistProvider*,int)), SIGNAL(providerRemoved(Playlists::PlaylistProvider*,int)) );
    connect( instance, SIGNAL(renamePlaylist(Playlists::PlaylistPtr)), SIGNAL(renamePlaylist(Playlists::PlaylistPtr)) );
    connect( instance, SIGNAL(updated(int)), SIGNAL(updated(int)) );
}

// script incokable

bool
AmarokPlaylistManagerScript::deletePlaylists( Playlists::PlaylistList playlistList )
{
    return PlaylistManager::instance()->deletePlaylists( playlistList );
}

QList< Playlists::PlaylistProvider* >
AmarokPlaylistManagerScript::getProvidersForPlaylist( const Playlists::PlaylistPtr playlist )
{
    return PlaylistManager::instance()->getProvidersForPlaylist( playlist );
}

bool
AmarokPlaylistManagerScript::import( const QString &fromLocation )
{
    return PlaylistManager::instance()->import( fromLocation );
}

bool
AmarokPlaylistManagerScript::isWritable( const Playlists::PlaylistPtr &playlist )
{
    return PlaylistManager::instance()->isWritable( playlist );
}

Playlists::PlaylistProvider*
AmarokPlaylistManagerScript::playlistProvider( int category, QString name )
{
    return PlaylistManager::instance()->playlistProvider( category, name );
}

Playlists::PlaylistList
AmarokPlaylistManagerScript::playlistsOfCategory( int playlistCategory )
{
    return PlaylistManager::instance()->playlistsOfCategory( playlistCategory );
}

PlaylistProviderList
AmarokPlaylistManagerScript::providersForCategory( int playlistCategory )
{
    return PlaylistManager::instance()->providersForCategory( playlistCategory );
}

bool
AmarokPlaylistManagerScript::rename( Playlists::PlaylistPtr playlist, const QString &newName )
{
    return PlaylistManager::instance()->rename( playlist, newName );
}

bool
AmarokPlaylistManagerScript::save( Meta::TrackList tracks, const QString &name, Playlists::PlaylistProvider *toProvider )
{
    return PlaylistManager::instance()->save( tracks, name, toProvider );
}

// private

QList<int>
AmarokPlaylistManagerScript::availableCategories()
{
    return PlaylistManager::instance()->availableCategories();
}
