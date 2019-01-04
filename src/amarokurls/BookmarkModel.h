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

#ifndef BOOKMARKMODEL_H
#define BOOKMARKMODEL_H

#include "BookmarkViewItem.h"

#include "AmarokSharedPointer.h"

#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>


class BookmarkGroup;

typedef AmarokSharedPointer<BookmarkViewItem> BookmarkViewItemPtr;

class BookmarkGroup;
typedef AmarokSharedPointer<BookmarkGroup> BookmarkGroupPtr;
typedef QList<BookmarkGroupPtr> BookmarkGroupList;


//#define BOOKMARK_DB_VERSION 1


/**
    @author Nikolaj Hald Nielsen <nhn@kde.org>
*/

class BookmarkModel : public QAbstractItemModel
{
    Q_OBJECT
public:

    enum Column
    {
        Name = 0,
        Command,
        Url,
        Description
    };

    static BookmarkModel * instance();

    ~BookmarkModel() override;

    QVariant data( const QModelIndex &index, int role ) const override;
    Qt::ItemFlags flags( const QModelIndex &index ) const override;
    QVariant headerData( int section, Qt::Orientation orientation,
                                 int role = Qt::DisplayRole ) const override;
    QModelIndex index( int row, int column,
                               const QModelIndex &parent = QModelIndex() ) const override;
    QModelIndex parent( const QModelIndex &index ) const override;
    int rowCount( const QModelIndex &parent = QModelIndex() ) const override;
    int columnCount( const QModelIndex &parent = QModelIndex() ) const override;
    bool setData( const QModelIndex &index, const QVariant &value, int role = Qt::EditRole ) override;

    Qt::DropActions supportedDropActions() const override
    {
        return Qt::MoveAction;
    }

    QStringList mimeTypes() const override;
    QMimeData* mimeData( const QModelIndexList &indexes ) const override;
    bool dropMimeData( const QMimeData * data, Qt::DropAction action, int row, int column, const QModelIndex &parent ) override;

    void reloadFromDb();
    void editBookmark( int id );

    QModelIndex createIndex( int row, int column, const BookmarkViewItemPtr &item ) const;
    //only use the above method
    QModelIndex createIndex( int, int, void * ptr = 0 ) const
    {
        Q_UNUSED( ptr );
        Q_ASSERT( 0 );
        return QModelIndex();
    }

    QModelIndex createIndex( int, int, quint32 ) const
    {
        Q_ASSERT( 0 );
        return QModelIndex();
    }

public Q_SLOTS:
    void createNewGroup();
    void createNewBookmark();
    void deleteBookmark( const QString &name );
    void renameBookmark( const QString &oldName , const QString &newName );

    /**
     * Sets the bookmark's (whose name is @param name ) url argument named @param key
     * to @param value . Overrides any possible previous value.
     */
    void setBookmarkArg(const QString &name, const QString &key, const QString &value);

Q_SIGNALS:
    void editIndex( const QModelIndex &index );

private:
    BookmarkModel();

    void checkTables();
    void createTables();
    void deleteTables();
    void upgradeTables( int from );

    bool setBookmarkArgRecursively( BookmarkGroupPtr group, const QString &name, const QString &key, const QString &value );
    bool deleteBookmarkRecursively( BookmarkGroupPtr group, const QString &name );
    bool renameBookmarkRecursively( BookmarkGroupPtr group, const QString &oldName, const QString &newName );

    static BookmarkModel * s_instance;

    BookmarkGroupPtr m_root;
    mutable QHash<quint32, BookmarkViewItemPtr> m_viewItems; ///the hash of the pointer mapped to the AmarokSharedPointer

};


#endif
