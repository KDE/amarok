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

#ifndef BOOKMARKGROUP_H
#define BOOKMARKGROUP_H

#include "AmarokUrl.h"
#include "BookmarkViewItem.h"

#include <QObject>
#include <QString>
#include <QStringList>

#include "AmarokSharedPointer.h"

class BookmarkGroup;
class AmarokUrl;

typedef AmarokSharedPointer<AmarokUrl> AmarokUrlPtr;
typedef AmarokSharedPointer<BookmarkGroup> BookmarkGroupPtr;
typedef QList<AmarokUrlPtr> BookmarkList;
typedef QList<BookmarkGroupPtr> BookmarkGroupList;


/**
A class for allowing a "folder structure" in the bookmark browser and the database. Takes care of reading and writing  itself to the database.
*/
class BookmarkGroup : public BookmarkViewItem
{
    public:
        BookmarkGroup( const QStringList &dbResultRow, const BookmarkGroupPtr &parent );
        explicit BookmarkGroup( const QString &name, const BookmarkGroupPtr &parent = BookmarkGroupPtr() );

        BookmarkGroup( const QString &name, const QString &customType );

        ~BookmarkGroup();

        int id() const;
        QString name() const override;
        QString description() const override;

        int childCount() const override;

        BookmarkGroupPtr parent() const override { return m_parent; }

        void rename( const QString &name ) override;
        void setDescription( const QString &description ) override;

        void save();
        BookmarkGroupList childGroups() const;
        BookmarkList childBookmarks() const;

        void reparent( const BookmarkGroupPtr &parent );

        void clear();

        void deleteChild( const BookmarkViewItemPtr &item );
        void removeFromDb() override;

    private:
        int m_dbId;
        BookmarkGroupPtr m_parent;
        QString m_name;
        QString m_description;
        QString m_customType;

        mutable BookmarkGroupList m_childGroups;
        mutable BookmarkList m_childBookmarks;

        mutable bool m_hasFetchedChildGroups;
        mutable bool m_hasFetchedChildPlaylists;
};

Q_DECLARE_METATYPE( BookmarkGroupPtr )
Q_DECLARE_METATYPE( AmarokUrlPtr )
Q_DECLARE_METATYPE( BookmarkList )
Q_DECLARE_METATYPE( BookmarkGroupList )

#endif
