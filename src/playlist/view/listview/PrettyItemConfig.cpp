/***************************************************************************
 *   Copyright (c) 2008  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 
#include "PrettyItemConfig.h"

namespace Playlist {

    PrettyItemConfigRowElement::PrettyItemConfigRowElement( int value, qreal size, bool bold, int alignment, const QString &string )
    : m_value( value )
    , m_size( size )
    , m_bold( bold )
    , m_alignment( alignment )
    , m_string( string )
{
}

int PrettyItemConfigRowElement::value() const
{
    return m_value;
}

qreal PrettyItemConfigRowElement::size() const
{
    return m_size;
}

bool PrettyItemConfigRowElement::bold() const
{
    return m_bold;
}

int PrettyItemConfigRowElement::alignment() const
{
    return m_alignment;
}

const QString &PrettyItemConfigRowElement::string() const
{
    return m_string;
}

//////////////////////////////////////////////

void PrettyItemConfigRow::addElement( PrettyItemConfigRowElement element )
{
    m_elements.append( element );
}

int PrettyItemConfigRow::count()
{
    return m_elements.count();
}

PrettyItemConfigRowElement PrettyItemConfigRow::element( int at )
{
    return m_elements.at( at );
}


//////////////////////////////////////////////

PrettyItemConfig::PrettyItemConfig()
    : m_showCover( false )
    , m_activeIndicatorRow( 0 )
{
}

PrettyItemConfig::~PrettyItemConfig()
{
}

int PrettyItemConfig::rows()
{
    return m_rows.size();
}


PrettyItemConfigRow Playlist::PrettyItemConfig::row( int at )
{
    return m_rows.at( at );
}

void Playlist::PrettyItemConfig::addRow( PrettyItemConfigRow row )
{
    m_rows.append( row );
}

bool PrettyItemConfig::showCover()
{
    return m_showCover;
}

void Playlist::PrettyItemConfig::setShowCover( bool showCover )
{
    m_showCover = showCover;
}



int Playlist::PrettyItemConfig::activeIndicatorRow()
{
    return m_activeIndicatorRow;
}

void Playlist::PrettyItemConfig::setActiveIndicatorRow( int row )
{
    m_activeIndicatorRow = row;
}

//////////////////////////////////////////////


PrettyItemConfig Playlist::PlaylistLayout::head()
{
    return m_head;
}

PrettyItemConfig Playlist::PlaylistLayout::body()
{
    return m_body;
}

PrettyItemConfig Playlist::PlaylistLayout::single()
{
    return m_single;
}

void Playlist::PlaylistLayout::setHead( PrettyItemConfig head )
{
    m_head = head;
}

void Playlist::PlaylistLayout::setBody( PrettyItemConfig body )
{
    m_body = body;
}

void Playlist::PlaylistLayout::setSingle( PrettyItemConfig single )
{
    m_single = single;
}

}



