/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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
 
#include "LayoutItemConfig.h"

namespace Playlist {

    LayoutItemConfigRowElement::LayoutItemConfigRowElement( int value, qreal size, bool bold, bool italic, Qt::Alignment alignment,
                                                            const QString &prefix , const QString &suffix )
    : m_value( value )
    , m_size( size )
    , m_bold( bold )
    , m_italic( italic )
    , m_alignment( alignment )
    , m_prefix( prefix )
    , m_suffix( suffix )
{
}

int LayoutItemConfigRowElement::value() const
{
    return m_value;
}

qreal LayoutItemConfigRowElement::size() const
{
    return m_size;
}

bool LayoutItemConfigRowElement::bold() const
{
    return m_bold;
}

bool Playlist::LayoutItemConfigRowElement::italic() const
{
    return m_italic;
}

Qt::Alignment LayoutItemConfigRowElement::alignment() const
{
    return m_alignment;
}

QString LayoutItemConfigRowElement::prefix() const
{
    return m_prefix;
}

QString LayoutItemConfigRowElement::suffix() const
{
    return m_suffix;
}

//////////////////////////////////////////////

void LayoutItemConfigRow::addElement( LayoutItemConfigRowElement element )
{
    m_elements.append( element );
}

int LayoutItemConfigRow::count()
{
    return m_elements.count();
}

LayoutItemConfigRowElement LayoutItemConfigRow::element( int at )
{
    return m_elements.at( at );
}


//////////////////////////////////////////////

LayoutItemConfig::LayoutItemConfig()
    : m_showCover( false )
    , m_activeIndicatorRow( 0 )
{
}

LayoutItemConfig::~LayoutItemConfig()
{
}

int LayoutItemConfig::rows() const
{
    return m_rows.size();
}


LayoutItemConfigRow Playlist::LayoutItemConfig::row( int at ) const
{
    return m_rows.at( at );
}

void Playlist::LayoutItemConfig::addRow( LayoutItemConfigRow row )
{
    m_rows.append( row );
}

bool LayoutItemConfig::showCover() const
{
    return m_showCover;
}

void Playlist::LayoutItemConfig::setShowCover( bool showCover )
{
    m_showCover = showCover;
}



int Playlist::LayoutItemConfig::activeIndicatorRow() const
{
    return m_activeIndicatorRow;
}

void Playlist::LayoutItemConfig::setActiveIndicatorRow( int row )
{
    m_activeIndicatorRow = row;
}

//////////////////////////////////////////////


Playlist::PlaylistLayout::PlaylistLayout()
    : m_isEditable(false)
    , m_isDirty(false)
    , m_inlineControls(false)
{}

LayoutItemConfig Playlist::PlaylistLayout::head() const
{
    return m_head;
}

LayoutItemConfig Playlist::PlaylistLayout::body() const
{
    return m_body;
}

LayoutItemConfig Playlist::PlaylistLayout::single() const
{
    return m_single;
}

void Playlist::PlaylistLayout::setHead( LayoutItemConfig head )
{
    m_head = head;
}

void Playlist::PlaylistLayout::setBody( LayoutItemConfig body )
{
    m_body = body;
}

void Playlist::PlaylistLayout::setSingle( LayoutItemConfig single )
{
    m_single = single;
}

bool Playlist::PlaylistLayout::isEditable() const
{
    return m_isEditable;
}

bool Playlist::PlaylistLayout::isDirty() const
{
    return m_isDirty;
}

void Playlist::PlaylistLayout::setEditable( bool editable )
{
    m_isEditable = editable;
}

void Playlist::PlaylistLayout::setDirty( bool dirty )
{
    m_isDirty = dirty;
}

bool Playlist::PlaylistLayout::inlineControls()
{
    return m_inlineControls;
}
void Playlist::PlaylistLayout::setInlineControls( bool inlineControls )
{
    m_inlineControls = inlineControls;
}

QString Playlist::PlaylistLayout::groupBy()
{
    return m_groupBy;
}

void Playlist::PlaylistLayout::setGroupBy(const QString& groupBy)
{
    m_groupBy = groupBy;
}

}







