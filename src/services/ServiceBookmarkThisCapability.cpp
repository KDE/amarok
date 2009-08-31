/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
#include "ServiceBookmarkThisCapability.h"

#include "ServiceMetaBase.h"

ServiceBookmarkThisCapability::ServiceBookmarkThisCapability( BookmarkThisProvider * provider )
    : Meta::BookmarkThisCapability()
    , m_provider( provider )
{
}


ServiceBookmarkThisCapability::~ServiceBookmarkThisCapability()
{
}

bool ServiceBookmarkThisCapability::isBookmarkable()
{
    return m_provider->isBookmarkable();
}

QString ServiceBookmarkThisCapability::browserName()
{
    return m_provider->browserName();
}

QString ServiceBookmarkThisCapability::collectionName()
{
    return m_provider->collectionName();
}

bool ServiceBookmarkThisCapability::simpleFiltering()
{
    return m_provider->simpleFiltering();
}

QAction * ServiceBookmarkThisCapability::bookmarkAction()
{
    return m_provider->bookmarkAction();
}


