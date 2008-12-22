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

#include "ScriptableServiceMeta.h"
#include "ScriptableServiceMeta_p.h"

#include "meta/PrivateMetaRegistry.h"

using namespace Meta;


ScriptableServiceMetaItem::ScriptableServiceMetaItem( int level )
    : m_callbackString( QString() )
    , m_level( level )
    , m_serviceName( QString() )
    , m_serviceDescription( QString() )
    , m_serviceEmblem( QPixmap() )
{}

void Meta::ScriptableServiceMetaItem::setCallbackString( const QString &callbackString )
{ 
    m_callbackString = callbackString;
}

QString Meta::ScriptableServiceMetaItem::callbackString() const
{
    return m_callbackString;
}

int Meta::ScriptableServiceMetaItem::level() const
{
    return m_level;
}

void Meta::ScriptableServiceMetaItem::setServiceName( const QString & name )
{
    m_serviceName = name;
}

void Meta::ScriptableServiceMetaItem::setServiceDescription( const QString & description )
{
    m_serviceDescription = description;
}

void Meta::ScriptableServiceMetaItem::setServiceEmblem( const QPixmap & emblem )
{
    m_serviceEmblem = emblem;
}



/* ScriptableServiceTrack */
ScriptableServiceTrack::ScriptableServiceTrack( const QString & name )
    : ServiceTrack( name )
    , ScriptableServiceMetaItem( 0 )
{}

ScriptableServiceTrack::ScriptableServiceTrack( const QStringList & resultRow )
    : ServiceTrack( resultRow )
    , ScriptableServiceMetaItem( 0 )
{}


Meta::AlbumPtr Meta::ScriptableServiceTrack::album() const
{
    DEBUG_BLOCK
    return m_albumPtr;
}

Meta::ArtistPtr Meta::ScriptableServiceTrack::artist() const
{
    DEBUG_BLOCK
    return m_artistPtr;
}

Meta::GenrePtr Meta::ScriptableServiceTrack::genre() const
{
    return m_genrePtr;
}

Meta::ComposerPtr Meta::ScriptableServiceTrack::composer() const
{
    return m_composerPtr;
}

Meta::YearPtr Meta::ScriptableServiceTrack::year() const
{
    return m_yearPtr;
}




void Meta::ScriptableServiceTrack::setAlbumName( const QString &newAlbum )
{
    Meta::AlbumPtr albumPtr = Meta::PrivateMetaRegistry::instance()->album( m_serviceName, newAlbum );
    if ( !albumPtr ) {
        albumPtr = Meta::AlbumPtr( new ScriptableServiceInternalAlbum( newAlbum ) );
        Meta::PrivateMetaRegistry::instance()->insertAlbum( m_serviceName, newAlbum, albumPtr );
    }

    m_albumPtr = albumPtr;
}

void Meta::ScriptableServiceTrack::setArtistName( const QString &newArtist )
{
    Meta::ArtistPtr artistPtr = Meta::PrivateMetaRegistry::instance()->artist( m_serviceName, newArtist );
    if ( !artistPtr ) {
        artistPtr = Meta::ArtistPtr( new ScriptableServiceInternalArtist( newArtist ) );
        Meta::PrivateMetaRegistry::instance()->insertArtist( m_serviceName, newArtist, artistPtr );
    }
    
    m_artistPtr = artistPtr;
}

void Meta::ScriptableServiceTrack::setGenreName( const QString &newGenre )
{
    Meta::GenrePtr genrePtr = Meta::PrivateMetaRegistry::instance()->genre( m_serviceName, newGenre );
    if ( !genrePtr ) {
        genrePtr = Meta::GenrePtr( new ScriptableServiceInternalGenre( newGenre ) );
        Meta::PrivateMetaRegistry::instance()->insertGenre( m_serviceName, newGenre, genrePtr );
    }
    
    m_genrePtr = genrePtr;
}

void Meta::ScriptableServiceTrack::setComposerName( const QString &newComposer )
{
    Meta::ComposerPtr composerPtr = Meta::PrivateMetaRegistry::instance()->composer( m_serviceName, newComposer );
    if ( !composerPtr ) {
        composerPtr = Meta::ComposerPtr( new ScriptableServiceInternalComposer( newComposer) );
        Meta::PrivateMetaRegistry::instance()->insertComposer( m_serviceName, newComposer, composerPtr );
    }
    
    m_composerPtr = composerPtr;
}

void Meta::ScriptableServiceTrack::setYearNumber( int newYear )
{
    const QString yearString = QString::number( newYear );
    
    Meta::YearPtr yearPtr = Meta::PrivateMetaRegistry::instance()->year( m_serviceName, yearString );
    if ( !yearPtr ) {
        yearPtr = Meta::YearPtr( new ScriptableServiceInternalYear( yearString ) );
        Meta::PrivateMetaRegistry::instance()->insertYear( m_serviceName, yearString, yearPtr );
    }
    
    m_yearPtr = yearPtr;
}

void Meta::ScriptableServiceTrack::setCustomAlbumCoverUrl( const QString & coverurl )
{
    DEBUG_BLOCK
            if ( m_albumPtr ) {
        debug() << "one";
        ServiceAlbumWithCoverPtr albumWithCover = ServiceAlbumWithCoverPtr::dynamicCast( m_albumPtr );
        if ( albumWithCover ) {
            debug() << "two";
            albumWithCover->setCoverUrl( coverurl );
        }
    }
}


QString Meta::ScriptableServiceTrack::sourceName()
{
    return m_serviceName;
}

QString Meta::ScriptableServiceTrack::sourceDescription()
{
    return m_serviceDescription;
}

QPixmap Meta::ScriptableServiceTrack::emblem()
{
    return m_serviceEmblem;
}




/* DynamicScriptableAlbum */
ScriptableServiceAlbum::ScriptableServiceAlbum( const QString & name )
    : ServiceAlbumWithCover( name )
    , ScriptableServiceMetaItem( 1 )
{}

ScriptableServiceAlbum::ScriptableServiceAlbum( const QStringList & resultRow )
    : ServiceAlbumWithCover( resultRow )
    , ScriptableServiceMetaItem( 1 )
{}


/* ScriptableServiceArtist */
ScriptableServiceArtist::ScriptableServiceArtist( const QString & name )
    : ServiceArtist( name )
    , ScriptableServiceMetaItem( 2 )
    , m_genreId( 0 )
{}

ScriptableServiceArtist::ScriptableServiceArtist( const QStringList & resultRow )
    : ServiceArtist( resultRow )
    , ScriptableServiceMetaItem( 2 )
    , m_genreId( 0 )
{}

void Meta::ScriptableServiceArtist::setGenreId(int genreId)
{
    m_genreId = genreId;
}


int Meta::ScriptableServiceArtist::genreId() const
{
    return m_genreId;
}

/* ScriptableServiceGenre */
ScriptableServiceGenre::ScriptableServiceGenre( const QString & name )
    : ServiceGenre( name )
    , ScriptableServiceMetaItem( 3 )
{}

ScriptableServiceGenre::ScriptableServiceGenre( const QStringList & resultRow )
    : ServiceGenre( resultRow )
    , ScriptableServiceMetaItem( 3 )
{}


void Meta::ScriptableServiceGenre::setDescription(const QString & description)
{
    m_description = description;
}

QString Meta::ScriptableServiceGenre::description()
{
    return m_description;
}





