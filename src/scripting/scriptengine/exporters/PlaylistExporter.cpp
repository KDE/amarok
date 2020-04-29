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

#include "PlaylistExporter.h"

#include "core/collections/QueryMaker.h"
#include "core/meta/Meta.h"
#include "core/playlists/PlaylistProvider.h"
#include "scripting/scriptengine/ScriptingDefines.h"

#include <QJSValue>
#include <core-impl/support/TrackLoader.h>

using namespace AmarokScript;

void
PlaylistPrototype::init( QJSEngine *engine )
{
    qScriptRegisterMetaType<Playlists::PlaylistPtr>( engine,
                                                     toScriptValue<Playlists::PlaylistPtr,PlaylistPrototype>,
                                                     fromScriptValue<Playlists::PlaylistPtr,PlaylistPrototype> );
    qScriptRegisterMetaType<Playlists::PlaylistList>( engine, toScriptArray, fromScriptArray );
}

// script invokable

void
PlaylistPrototype::triggerFullLoad()
{
    TrackLoader *loader = new TrackLoader( TrackLoader::FullMetadataRequired );
    loader->init( Playlists::PlaylistList() << m_playlist );
    connect( loader, SIGNAL(finished(Meta::TrackList)), SIGNAL(loaded(Meta::TrackList)) );
}

void
PlaylistPrototype::triggerQuickLoad()
{
    TrackLoader *loader = new TrackLoader();
    connect( loader, SIGNAL(finished(Meta::TrackList)), SIGNAL(loaded(Meta::TrackList)) );
}

void
PlaylistPrototype::trackAdded(const Playlists::PlaylistPtr &playlist, const Meta::TrackPtr &track, int position )
{
    Q_UNUSED( playlist )
    Q_EMIT addedTrack( track, position );
}

void
PlaylistPrototype::trackRemoved(const Playlists::PlaylistPtr &playlist, int position )
{
    Q_UNUSED( playlist )
    Q_EMIT removedTrack( position );
}

void
PlaylistPrototype::addTrack( Meta::TrackPtr track, int position )
{
    if( m_playlist )
        m_playlist->addTrack( track, position );
}

Playlists::PlaylistProvider*
PlaylistPrototype::provider() const
{
    if( m_playlist )
        return m_playlist->provider();
    return nullptr;
}

void
PlaylistPrototype::removeTrack( int position )
{
    if( m_playlist )
        m_playlist->removeTrack( position );
}

Meta::TrackList
PlaylistPrototype::tracks()
{
    if( m_playlist )
        return m_playlist->tracks();
    return Meta::TrackList();
}

// private

PlaylistPrototype::PlaylistPrototype( Playlists::PlaylistPtr playlist )
: QObject( nullptr )
, m_playlist( playlist )
{
    PlaylistObserver::subscribeTo( playlist );
}

bool
PlaylistPrototype::isValid() const
{
    return m_playlist;
}

QString
PlaylistPrototype::toString() const
{
    if( !m_playlist )
        return QStringLiteral( "Invalid" );
    return m_playlist->name();
}

void
PlaylistPrototype::setName( const QString &name )
{
    if( m_playlist )
        m_playlist->setName( name );
}

QUrl
PlaylistPrototype::uidUrl() const
{
    if( !m_playlist )
        return QUrl();
    return m_playlist->uidUrl();
}

int
PlaylistPrototype::trackCount() const
{
    return m_playlist->trackCount();
}
