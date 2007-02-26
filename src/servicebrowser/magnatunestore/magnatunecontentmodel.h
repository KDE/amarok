 /*
  Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/


#ifndef AMAROKMAGNATUNEMODEL_H
#define AMAROKMAGNATUNEMODEL_H

#include "magnatunetypes.h"

#include <QAbstractItemModel>
#include <QMap>
#include <QModelIndex>
#include <QVariant>

#include "magnatunecontentitem.h"

class MagnatuneContentModel : public QAbstractItemModel
{
    Q_OBJECT

private:

   /* mutable QMap<int, MagnatuneDatabaseEntry *> m_artistEntriesMap;
    mutable QMap<int, MagnatuneDatabaseEntry *> m_albumEntriesMap;
    mutable QMap<int, MagnatuneDatabaseEntry *> m_trackEntriesMap;
*/
    private:

    MagnatuneContentItem *m_rootContentItem;

public:
    
    MagnatuneContentModel(QObject *parent = 0);
    
    ~MagnatuneContentModel();

    QVariant data(const QModelIndex &index, int role) const;
     
    Qt::ItemFlags flags(const QModelIndex &index) const;
    
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    
    QModelIndex parent(const QModelIndex &index) const;
    
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    
    int columnCount(const QModelIndex &parent = QModelIndex()) const;

 };

 #endif