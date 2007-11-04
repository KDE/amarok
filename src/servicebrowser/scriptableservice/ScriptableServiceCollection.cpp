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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.         *
 ***************************************************************************/

#include "ScriptableServiceCollection.h"
#include "support/MemoryQueryMaker.h"


ScriptableServiceCollection::ScriptableServiceCollection(const QString & name)
    : ServiceCollection()
{
    m_name = name;
}


ScriptableServiceCollection::~ScriptableServiceCollection()
{
}

QueryMaker*
ScriptableServiceCollection::queryMaker()
{
    return new MemoryQueryMaker( this, collectionId() );
}

QString ScriptableServiceCollection::prettyName() const
{
    return m_name;
}

QString ScriptableServiceCollection::collectionId() const
{
    return m_name;
}


bool ScriptableServiceCollection::possiblyContainsTrack(const KUrl & url) const
{
    Q_UNUSED( url );
    return false;
}

Meta::TrackPtr ScriptableServiceCollection::ScriptableServiceCollection::trackForUrl(const KUrl & url)
{
    Q_UNUSED( url );
    return Meta::TrackPtr();
}

CollectionLocation * ScriptableServiceCollection::ScriptableServiceCollection::location() const
{
    return 0;
}


