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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "AmpacheServiceCollection.h"

#include "AmpacheServiceQueryMaker.h"

AmpacheServiceCollection::AmpacheServiceCollection( const QString &server, const QString &sessionId )
    : ServiceDynamicCollection( "AmpacheCollection", "AmpacheCollection" )
    , m_server( server )
    , m_sessionId( sessionId )
{
}


AmpacheServiceCollection::~AmpacheServiceCollection()
{
}

QueryMaker * AmpacheServiceCollection::queryMaker()
{
    return new AmpacheServiceQueryMaker( this, m_server, m_sessionId );
}

QString AmpacheServiceCollection::collectionId() const
{
    return "Mp3Tunes collection";
}

QString AmpacheServiceCollection::prettyName() const
{
    return collectionId();
}


