/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
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

#include "PlaylistBreadcrumbLevel.h"

#include "PlaylistDefines.h"

#include <KLocale>

namespace Playlist
{

BreadcrumbLevel::BreadcrumbLevel( QString internalColumnName )
    : m_name( internalColumnName )
{
    Column col = columnForName( internalColumnName );

    if( col == Shuffle )
    {
        m_icon = KIcon( "media-playlist-shuffle" );
        m_prettyName = i18n( "Shuffle" );
    }
    else
    {
        m_icon = KIcon( iconName( col ) );
        m_prettyName = columnName( col );
    }

    for( int i = 0; i < NUM_COLUMNS; ++i )  //might be faster if it used a const_iterator
    {
        Column currentCol = static_cast<Column>(i);
        if( !isSortableColumn( currentCol ) || currentCol == col )
            continue;
        m_siblings.insert( Playlist::internalColumnName( currentCol ),
                           QPair< KIcon, QString>( KIcon( iconName( currentCol ) ), columnName( currentCol ) ) );
    }
    if( col != Shuffle )
        m_siblings.insert( "Shuffle", QPair< KIcon, QString>( KIcon( "media-playlist-shuffle" ), i18n("Shuffle" ) ) );
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
