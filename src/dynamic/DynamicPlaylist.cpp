/****************************************************************************************
 * Copyright (c) 2008 Daniel Jones <danielcjones@gmail.com>                             *
 * Copyright (c) 2010 Ralf Engels <ralf-engels@gmx.de>                                  *
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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "DynamicPlaylist.h"
#include "DynamicModel.h"

Dynamic::DynamicPlaylist::DynamicPlaylist( QObject *parent )
    : QObject( parent )
{
}

Dynamic::DynamicPlaylist::~DynamicPlaylist()
{
}

/*
void
Dynamic::DynamicPlaylist::repopulate()
{
    // do nothing by default
}
*/

QString Dynamic::DynamicPlaylist::title() const
{ return m_title; }

void Dynamic::DynamicPlaylist::setTitle( const QString &title )
{
    m_title = title;
    bool inModel = DynamicModel::instance()->index( this ).isValid();
    if( inModel )
        DynamicModel::instance()->playlistChanged( this );
}


