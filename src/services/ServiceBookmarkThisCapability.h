/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
 
#ifndef SERVICEBOOKMARKTHISCAPABILITY_H
#define SERVICEBOOKMARKTHISCAPABILITY_H

#include "amarok_export.h"
#include "meta/capabilities/BookmarkThisCapability.h"

class BookmarkThisProvider;

/**
A service specific implementation of the BookmarkThisCapability

	@author Nikolaj Hald Nielsen <nhn@kde.org> 
*/
class AMAROK_EXPORT ServiceBookmarkThisCapability : public Meta::BookmarkThisCapability {
public:
    ServiceBookmarkThisCapability( BookmarkThisProvider * provider );

    ~ServiceBookmarkThisCapability();

    virtual bool isBookmarkable();
    virtual QString browserName();
    virtual QString collectionName();
    virtual bool simpleFiltering();
    virtual QAction * bookmarkAction();

private:

    BookmarkThisProvider * m_provider;
};

#endif
