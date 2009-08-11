/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "PlaylistBreadcrumbLevel.h"

#include "PlaylistDefines.h"

namespace Playlist
{

BreadcrumbLevel::BreadcrumbLevel( QString internalColumnName )
    : m_name( internalColumnName )
{
    if( m_name == "random" )
    {
        m_icon = KIcon( "media-playlist-shuffle" );
        m_prettyName = i18n( "Random" );
    }
    else
    {
        m_icon = KIcon( iconNames.at( internalColumnNames.indexOf( internalColumnName ) ) );
        m_prettyName = columnNames.at( internalColumnNames.indexOf( internalColumnName ) );
    }
    
    for( int i = 0; i < NUM_COLUMNS; ++i )  //might be faster if it used a const_iterator
    {
        QString currentInternalColumnName = internalColumnNames.at( i );
        if( !sortableCategories.contains( currentInternalColumnName ) ||
            m_name == currentInternalColumnName )
            continue;
        m_siblings.insert( currentInternalColumnName,
                           QPair< KIcon, QString>( KIcon( iconNames.at( i ) ), columnNames.at( i ) ) );
    }
    if( m_name != "random" )
        m_siblings.insert( "random", QPair< KIcon, QString>( KIcon( "media-playlist-shuffle" ), i18n("Random" ) ) );
}

BreadcrumbLevel::~BreadcrumbLevel()
{}

const QString &
BreadcrumbLevel::name()
{
    return m_name;
}

const QString &
BreadcrumbLevel::prettyName()
{
    return m_prettyName;
}

const KIcon &
BreadcrumbLevel::icon()
{
    return m_icon;
}

const QMap< QString, QPair< KIcon, QString > >
BreadcrumbLevel::siblings()
{
    return m_siblings;
}

}   //namespace Playlist
