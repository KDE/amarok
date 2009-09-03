/****************************************************************************************
 * Copyright (c) 2008 Andreas Muetzel <andreas.muetzel@gmx.net>                         *
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
 
#ifndef AMAROK_ALBUMSMODEL_H
#define AMAROK_ALBUMSMODEL_H
 
#include <QStandardItemModel>

/**
 * This Model is used to get the right mime type/data for entries in the albums treeview
 */
class AlbumsModel : public QStandardItemModel
{
     public:
         virtual QMimeData* mimeData(const QModelIndexList & indices) const;
         virtual QMimeData* mimeData(const QList<QStandardItem*> & items) const;
         virtual QStringList mimeTypes() const;
};
 
#endif
