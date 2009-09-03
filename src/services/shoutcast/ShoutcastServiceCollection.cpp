/****************************************************************************************
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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

#include "ShoutcastServiceCollection.h"

#include "ShoutcastServiceQueryMaker.h"

ShoutcastServiceCollection::ShoutcastServiceCollection()
    : ServiceCollection( 0, "Shoutcast.com", "Shoutcast.com" )
{
    m_top500 = false;
}

ShoutcastServiceCollection::ShoutcastServiceCollection( bool isTop500 )
    : ServiceCollection( 0, "Shoutcast.com", "Shoutcast.com" )
{
    m_top500 = isTop500;
}


ShoutcastServiceCollection::~ShoutcastServiceCollection()
{
    DEBUG_BLOCK
}

QueryMaker * ShoutcastServiceCollection::queryMaker()
{
    return new ShoutcastServiceQueryMaker( this, m_top500 );
}

QString ShoutcastServiceCollection::collectionId() const
{
    return "Shoutcast collection";
}

QString ShoutcastServiceCollection::prettyName() const
{
    return collectionId();
}


