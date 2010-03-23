/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "PrivateMetaRegistry.h"

#include "Debug.h"

namespace Meta {


PrivateMetaRegistry* PrivateMetaRegistry::s_instance = 0;


PrivateMetaRegistry * PrivateMetaRegistry::instance()
{
    if ( s_instance == 0 )
        s_instance = new PrivateMetaRegistry();
    return s_instance;
}


PrivateMetaRegistry::PrivateMetaRegistry()
{
}


PrivateMetaRegistry::~PrivateMetaRegistry()
{
}


void PrivateMetaRegistry::insertAlbum( const QString &owner, const QString &key, AlbumPtr album )
{
    const QString compositeKey = owner + '-' + key;
    m_albums.insert( compositeKey, album );
}

void PrivateMetaRegistry::insertArtist( const QString &owner, const QString &key, ArtistPtr artist )
{
    const QString compositeKey = owner + '-' + key;
    m_artists.insert( compositeKey, artist );
}

void PrivateMetaRegistry::insertGenre( const QString &owner, const QString &key, GenrePtr genre )
{
    const QString compositeKey = owner + '-' + key;
    m_genre.insert( compositeKey, genre );
}

void PrivateMetaRegistry::insertComposer( const QString &owner, const QString &key, ComposerPtr composer )
{
    const QString compositeKey = owner + '-' + key;
    m_composers.insert( compositeKey, composer );
}

void PrivateMetaRegistry::insertYear( const QString &owner, const QString &key, YearPtr year )
{
    const QString compositeKey = owner + '-' + key;
    m_years.insert( compositeKey, year );
}

AlbumPtr PrivateMetaRegistry::album( const QString &owner, const QString &key )
{
    DEBUG_BLOCK
    const QString compositeKey = owner + '-' + key;
    if ( m_albums.contains( compositeKey ) ) {
        debug() << "reusing album with key: " << key;
         return m_albums.value( compositeKey );

    }
    return AlbumPtr();
}

ArtistPtr PrivateMetaRegistry::artist( const QString &owner, const QString &key )
{
    const QString compositeKey = owner + '-' + key;
    if ( m_artists.contains( compositeKey ) )
        return m_artists.value( compositeKey );
    return ArtistPtr();
}

GenrePtr PrivateMetaRegistry::genre( const QString &owner, const QString &key )
{
    const QString compositeKey = owner + '-' + key;
    if ( m_genre.contains( compositeKey ) )
        return m_genre.value( compositeKey );
    return GenrePtr();
}

ComposerPtr PrivateMetaRegistry::composer( const QString &owner, const QString &key )
{
    const QString compositeKey = owner + '-' + key;
    if ( m_composers.contains( compositeKey ) )
        return m_composers.value( compositeKey );
    return ComposerPtr();
}

YearPtr PrivateMetaRegistry::year( const QString &owner, const QString &key )
{
     const QString compositeKey = owner + '-' + key;
     if ( m_years.contains( compositeKey ) )
         return m_years.value( compositeKey );
     return YearPtr();
}

}

