/****************************************************************************************
 * Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) version 3 or        *
 * any later version accepted by the membership of KDE e.V. (or its successor approved  *
 * by the membership of KDE e.V.), which shall act as a proxy defined in Section 14 of  *
 * version 3 of the license.                                                            *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "DynamicPlaylist.h"
#include "CollectionManager.h"



Dynamic::DynamicPlaylist::DynamicPlaylist( Amarok::Collection* coll )
     : m_collection(coll)
{
    if( !m_collection )
    {
        m_collection = CollectionManager::instance()->primaryCollection();
        connect( m_collection, SIGNAL(updated()), this, SLOT(recalculate()) );
    }
}

Dynamic::DynamicPlaylist::~DynamicPlaylist()
{
}

QDomElement
Dynamic::DynamicPlaylist::xml() const
{
    // unsaveable by default.
    return QDomElement();
}


void
Dynamic::DynamicPlaylist::recalculate()
{
    // do nothing by default
}


QString Dynamic::DynamicPlaylist::title() const     { return m_title; }

void Dynamic::DynamicPlaylist::setTitle( QString title )        { m_title = title; }


