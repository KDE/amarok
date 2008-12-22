//
// C++ Implementation: PrivateMetaRegistry
//
// Description: 
//
//
// Author:  <>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
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
    const QString compositeKey = owner + "-" + key;
    m_albums.insert( compositeKey, album );
}

void PrivateMetaRegistry::insertArtist( const QString &owner, const QString &key, ArtistPtr artist )
{
    const QString compositeKey = owner + "-" + key;
    m_artists.insert( compositeKey, artist );
}

void PrivateMetaRegistry::insertGenre( const QString &owner, const QString &key, GenrePtr genre )
{
    const QString compositeKey = owner + "-" + key;
    m_genre.insert( compositeKey, genre );
}

void PrivateMetaRegistry::insertComposer( const QString &owner, const QString &key, ComposerPtr composer )
{
    const QString compositeKey = owner + "-" + key;
    m_composers.insert( compositeKey, composer );
}

void PrivateMetaRegistry::insertYear( const QString &owner, const QString &key, YearPtr year )
{
    const QString compositeKey = owner + "-" + key;
    m_years.insert( compositeKey, year );
}

AlbumPtr PrivateMetaRegistry::album( const QString &owner, const QString &key )
{
    DEBUG_BLOCK
    const QString compositeKey = owner + "-" + key;
    if ( m_albums.contains( compositeKey ) ) {
        debug() << "reusing album with key: " << key;
         return m_albums.value( compositeKey );

    }
    return AlbumPtr();
}

ArtistPtr PrivateMetaRegistry::artist( const QString &owner, const QString &key )
{
    const QString compositeKey = owner + "-" + key;
    if ( m_artists.contains( compositeKey ) )
        return m_artists.value( compositeKey );
    return ArtistPtr();
}

GenrePtr PrivateMetaRegistry::genre( const QString &owner, const QString &key )
{
    const QString compositeKey = owner + "-" + key;
    if ( m_genre.contains( compositeKey ) )
        return m_genre.value( compositeKey );
    return GenrePtr();
}

ComposerPtr PrivateMetaRegistry::composer( const QString &owner, const QString &key )
{
    const QString compositeKey = owner + "-" + key;
    if ( m_composers.contains( compositeKey ) )
        return m_composers.value( compositeKey );
    return ComposerPtr();
}

YearPtr PrivateMetaRegistry::year( const QString &owner, const QString &key )
{
     const QString compositeKey = owner + "-" + key;
     if ( m_years.contains( compositeKey ) )
         return m_years.value( compositeKey );
     return YearPtr();
}

}

