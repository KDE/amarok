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
     , d( new ScriptableServiceTrack::Private( this ) )
{}

ScriptableServiceTrack::ScriptableServiceTrack( const QStringList & resultRow )
    : ServiceTrack( resultRow )
    , ScriptableServiceMetaItem( 0 )
    , d( new ScriptableServiceTrack::Private( this ) )
{}


Meta::AlbumPtr Meta::ScriptableServiceTrack::album() const
{
    DEBUG_BLOCK
    if ( d->album ){
        debug() << "got a custom album named " << d->album->name();
        return d->album;
    }
    return ServiceTrack::album();
}

Meta::ArtistPtr Meta::ScriptableServiceTrack::artist() const
{
    DEBUG_BLOCK
            
    if ( d->artist ) {
        debug() << "got a custom artist named " << d->artist->name();
        return d->artist;
    }
    return ServiceTrack::artist();
}

Meta::GenrePtr Meta::ScriptableServiceTrack::genre() const
{
    if ( d->genre )
        return d->genre;
    return ServiceTrack::genre();
}

Meta::ComposerPtr Meta::ScriptableServiceTrack::composer() const
{
    if ( d->composer )
        return d->composer;
    return ServiceTrack::composer();
}

Meta::YearPtr Meta::ScriptableServiceTrack::year() const
{
    if ( d->year )
        return d->year;
    return ServiceTrack::year();
}




void Meta::ScriptableServiceTrack::setAlbumName( const QString &newAlbum )
{
    d->m_data.album = newAlbum;
    d->album = Meta::AlbumPtr( new ScriptableServiceInternalAlbum( QPointer<ScriptableServiceTrack::Private>( d ) ) );
}

void Meta::ScriptableServiceTrack::setArtistName( const QString &newArtist )
{
    d->m_data.artist = newArtist;
    d->artist = Meta::ArtistPtr( new ScriptableServiceInternalArtist( QPointer<ScriptableServiceTrack::Private>( d ) ) );
}

void Meta::ScriptableServiceTrack::setGenreName( const QString &newGenre )
{
    d->m_data.genre = newGenre;
    d->genre = Meta::GenrePtr( new ScriptableServiceInternalGenre( QPointer<ScriptableServiceTrack::Private>( d ) ) );
}

void Meta::ScriptableServiceTrack::setComposerName( const QString &newComposer )
{
    d->m_data.composer = newComposer;
    d->composer = Meta::ComposerPtr( new ScriptableServiceInternalComposer( QPointer<ScriptableServiceTrack::Private>( d ) ) );
}

void Meta::ScriptableServiceTrack::setYearNumber( int newYear )
{
    d->m_data.year = newYear;
    d->year = Meta::YearPtr( new ScriptableServiceInternalYear( QPointer<ScriptableServiceTrack::Private>( d ) ) );
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
    : ServiceAlbum( name )
    , ScriptableServiceMetaItem( 1 )
{}

ScriptableServiceAlbum::ScriptableServiceAlbum( const QStringList & resultRow )
    : ServiceAlbum( resultRow )
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


