/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
 
#ifndef BOOKMARKVIEWITEM_H
#define BOOKMARKVIEWITEM_H

#include "Debug.h"

#include <QSharedData>
#include <KSharedPtr>
class BookmarkGroup;

typedef KSharedPtr<BookmarkGroup> BookmarkGroupPtr;
typedef QList<BookmarkGroupPtr> BookmarkGroupList;

/**
	@author Nikolaj Hald Nielsen <nhn@kde.org> 
*/

class BookmarkViewItem;
typedef KSharedPtr<BookmarkViewItem> BookmarkViewItemPtr;

class BookmarkViewItem : public virtual QSharedData
{
    public:
        BookmarkViewItem() : QSharedData() {}
        
        virtual  ~BookmarkViewItem() { DEBUG_BLOCK };
    
        virtual BookmarkGroupPtr parent() const = 0;
        virtual int childCount() const { return 0; }
        virtual QString name() const = 0;
        virtual QString description() const = 0;
        virtual void rename( const QString &name ) = 0;
        virtual void setDescription( const QString &description ) = 0;
        virtual void removeFromDb() = 0;

};

Q_DECLARE_METATYPE( BookmarkViewItemPtr )

#endif
