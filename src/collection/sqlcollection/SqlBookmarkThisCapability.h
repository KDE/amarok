/***************************************************************************
 *   Copyright (c) 2009  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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
 
#ifndef METASQLBOOKMARKTHISCAPABILITY_H
#define METASQLBOOKMARKTHISCAPABILITY_H

#include "meta/capabilities/BookmarkThisCapability.h"
#include "SqlMeta.h"

namespace Meta
{

/**
    @author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class SqlBookmarkThisCapability : public BookmarkThisCapability
{
    Q_OBJECT
public:
    SqlBookmarkThisCapability( PopupDropperAction * action );

    virtual ~SqlBookmarkThisCapability();

    virtual bool isBookmarkable() { return true; }
    virtual QString browserName() { return "collection"; }
    virtual QString collectionName() { return QString(); }
    virtual bool simpleFiltering() { return false; }
    virtual PopupDropperAction * bookmarkAction();

private:

    PopupDropperAction * m_action;
};

}

#endif
