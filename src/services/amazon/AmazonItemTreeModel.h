/****************************************************************************************
 * Copyright (c) 2011 Sven Krohlas <sven@asbest-online.de>                              *
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

#ifndef AMAZONITEMTREEMODEL_H
#define AMAZONITEMTREEMODEL_H

#include "AmazonCollection.h"

#include <QAbstractTableModel>

class AmazonItemTreeModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    AmazonItemTreeModel( Collections::AmazonCollection* collection );

    // Reimplemented from QAbstractItemModel
    virtual int columnCount( const QModelIndex &parent ) const;
    virtual QVariant data( const QModelIndex &index, int role ) const;
    virtual Qt::ItemFlags flags( const QModelIndex &index ) const;
    virtual QVariant headerData( int section, Qt::Orientation orientation, int role ) const;
    virtual QMimeData* mimeData( const QModelIndexList &indices ) const;
    virtual QStringList mimeTypes() const;
    virtual int rowCount( const QModelIndex &parent ) const;

    /**
    * Given a QModelIndex this returns the ID the item has in the collection.
    * Use isAlbum() to check weather it's an album or a track.
    * @param index the QModelIndex to check.
    * @return the ID the item can be found in the collection, -1 if index is invalid.
    */
    int idForIndex( const QModelIndex &index ) const;

    /**
    * Checks if the item at the specified index is an album or not.
    * Use idForIndex() to get the ID of the item.
    * @param index the QModelIndex to check.
    * @return true if album, false if track.
    */
    bool isAlbum( const QModelIndex &index ) const;

private:
    Collections::AmazonCollection* m_collection;
    int m_hiddenAlbums;

    /**
    * Helper function. Returns the pretty name for the item at the specified index.
    * @param index the QModelIndex to generate the pretty name from.
    * @return pretty name of the item at the specified index.
    */
    QString prettyNameByIndex( const QModelIndex &index ) const;

private Q_SLOTS:
    void collectionChanged();
};

#endif // AMAZONITEMTREEMODEL_H
