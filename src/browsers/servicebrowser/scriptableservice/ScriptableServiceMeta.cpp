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
ScriptableServiceTrack::ScriptableServiceTrack( const QString & name, const QString & url )
    : ServiceTrack( name, url )
    , ScriptableServiceMetaItem( 0 )
{}

ScriptableServiceTrack::ScriptableServiceTrack( const QStringList & resultRow )
    : ServiceTrack( resultRow )
    , ScriptableServiceMetaItem( 0 )
{}

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
{}

ScriptableServiceArtist::ScriptableServiceArtist( const QStringList & resultRow )
    : ServiceArtist( resultRow )
    , ScriptableServiceMetaItem( 2 )
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




