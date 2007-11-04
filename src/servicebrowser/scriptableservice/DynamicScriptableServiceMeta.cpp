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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.         *
 ***************************************************************************/

#include "DynamicScriptableServiceMeta.h"

using namespace Meta;


DynamicScriptableMetaItem::DynamicScriptableMetaItem() 
    : m_callbackString( QString() )
    , m_level( 0 )
{}

void Meta::DynamicScriptableMetaItem::setCallbackString(QString callbackString)
{ 
    m_callbackString = callbackString;
}

QString Meta::DynamicScriptableMetaItem::callbackString() const
{
    return m_callbackString;
}


void Meta::DynamicScriptableMetaItem::setLevel(int level)
{
    m_level = level;
}

int Meta::DynamicScriptableMetaItem::level() const
{
    return m_level;
}



/* DynamicScriptableTrack */
DynamicScriptableTrack::DynamicScriptableTrack( const QString & name )
    : ServiceTrack( name )
{}

DynamicScriptableTrack::DynamicScriptableTrack( const QStringList & resultRow )
    : ServiceTrack( resultRow )
{}




/* DynamicScriptableAlbum */
DynamicScriptableAlbum::DynamicScriptableAlbum( const QString & name )
    : ServiceAlbum( name )
    , DynamicScriptableMetaItem()
{}

DynamicScriptableAlbum::DynamicScriptableAlbum( const QStringList & resultRow )
    : ServiceAlbum( resultRow )
    , DynamicScriptableMetaItem()
{}



/* DynamicScriptableArtist */
DynamicScriptableArtist::DynamicScriptableArtist( const QString & name )
    : ServiceArtist( name )
    , DynamicScriptableMetaItem()
{}

DynamicScriptableArtist::DynamicScriptableArtist( const QStringList & resultRow )
    : ServiceArtist( resultRow )
    , DynamicScriptableMetaItem()
{}


/* DynamicScriptableGenre */
DynamicScriptableGenre::DynamicScriptableGenre( const QString & name )
    : ServiceGenre( name )
    , DynamicScriptableMetaItem()
{}

DynamicScriptableGenre::DynamicScriptableGenre( const QStringList & resultRow )
    : ServiceGenre( resultRow )
    , DynamicScriptableMetaItem()
{}





