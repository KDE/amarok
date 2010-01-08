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

#include "ScriptableServiceCollection.h"

#include "Debug.h"
#include "ScriptableServiceQueryMaker.h"

ScriptableServiceCollection::ScriptableServiceCollection( const QString &name )
    : ServiceCollection( 0, name, name )
{
    DEBUG_BLOCK
    m_name = name;
}


ScriptableServiceCollection::~ScriptableServiceCollection()
{
}

QueryMaker * ScriptableServiceCollection::queryMaker()
{
    return new ScriptableServiceQueryMaker( this, m_name );
}

QString ScriptableServiceCollection::collectionId() const
{
    return "Scriptable Service collection";
}

QString ScriptableServiceCollection::prettyName() const
{
    return collectionId();
}

void ScriptableServiceCollection::donePopulating( int parentId )
{
    DEBUG_BLOCK
    Q_UNUSED( parentId );
    emit updateComplete();
}

void ScriptableServiceCollection::clear()
{
    acquireWriteLock();
    genreMap().clear();
    setGenreMap( GenreMap() );
    artistMap().clear();
    setArtistMap( ArtistMap() );
    albumMap().clear();
    setAlbumMap( AlbumMap() );
    trackMap().clear();
    setTrackMap( TrackMap() );
    releaseLock();
}

