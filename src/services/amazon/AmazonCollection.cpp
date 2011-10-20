/****************************************************************************************
 * Copyright (c) 2011 Sven Krohlas <sven@getamarok.com>                                 *
 * The Amazon store in based upon the Magnatune store in Amarok,                        *
 * Copyright (c) 2006,2007 Nikolaj Hald Nielsen <nhn@kde.org>                           *
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

#include "AmazonCollection.h"

Collections::AmazonCollection::AmazonCollection( ServiceBase * service, const QString &id, const QString &prettyName )
    : ServiceCollection( service, id, prettyName )
{
}

QMap<QString, int> *Collections::AmazonCollection::artistIDMap()
{
    return &m_artistIDMap;
}

QMap<QString, int> *Collections::AmazonCollection::albumIDMap()
{
    return &m_albumIDMap;
}

QMap<QString, int> *Collections::AmazonCollection::trackIDMap()
{
    return &m_trackIDMap;
}

void Collections::AmazonCollection::clear()
{
    m_artistIDMap.clear();
    m_albumIDMap.clear();
    m_trackIDMap.clear();
}
