/****************************************************************************************
 * Copyright (c) 2010 Casey Link <unnamedrambler@gmail.com>                             *
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

#ifndef MIMETYPEFILTERPROXYMODEL_H
#define MIMETYPEFILTERPROXYMODEL_H

#include <QSortFilterProxyModel>

#include <QStringList>

/**
 * @class MimeTypeFilterProxyModel a proxy model to filter out KFileItem's that do not match a supplied set of mimetypes
 * Designed to be used with KDirOperator as it uses KDirModel::FileItemRole to retrieve the KFileItem
 */
class MimeTypeFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    /**
     * MimeTypeFilterProxyModel
     * @param mimeList the valid mimetypes
     */
     explicit MimeTypeFilterProxyModel( QStringList mimeList, QObject *parent = 0 );


protected:
    virtual bool filterAcceptsRow( int source_row, const QModelIndex& source_parent ) const;
    virtual bool lessThan( const QModelIndex& left, const QModelIndex& right ) const;

private:
    QStringList m_mimeList;
};

#endif // MIMETYPEFILTERPROXYMODEL_H
