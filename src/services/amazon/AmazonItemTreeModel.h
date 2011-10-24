/****************************************************************************************
 * Copyright (c) 2011 Sven Krohlas <sven@getamarok.com>                                 *
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

    virtual Qt::ItemFlags flags( const QModelIndex &index ) const;

    int rowCount( const QModelIndex &parent ) const;
    int columnCount( const QModelIndex &parent ) const;
    QVariant headerData( int section, Qt::Orientation orientation, int role ) const;
    QVariant data( const QModelIndex &index, int role ) const;
    void collectionChanged();
    bool isAlbum( const QModelIndex &index ) const;
    virtual QStringList mimeTypes() const;
    virtual QMimeData* mimeData( const QModelIndexList &indices ) const;

private:
    Collections::AmazonCollection* m_collection;
};

#endif // AMAZONITEMTREEMODEL_H
