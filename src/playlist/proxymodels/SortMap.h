/***************************************************************************
 *   Copyright © 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                *
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

#ifndef AMAROK_PLAYLISTSORTMAP_H
#define AMAROK_PLAYLISTSORTMAP_H

#include "SortScheme.h"

#include <QVector>

namespace Playlist
{

/**
 *
 */
class SortMap
{
public:
    SortMap( qint64 rowCount );

    qint64 inv( qint64 proxyRow );

    qint64 map( qint64 sourceRow );

    void insertRows( qint64 startRowInSource, qint64 rowCount );

    bool isSorted(){ return m_sorted; };

    void setRowCount( qint64 rowCount ){ m_rowCount = rowCount; };

private:
    QMap< qint64, qint64 > *m_map;
    qint64 m_rowCount;
    bool m_sorted;
};

}   //namespace Playlist

#endif  //AMAROK_PLAYLISTSORTMAP_H
