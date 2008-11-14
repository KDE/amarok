/***************************************************************************
 *   Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>             *
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

#include "ServiceListSortFilterProxyModel.h"
#include "ServiceListModel.h"

#include "Debug.h"

#include <QVariant>

ServiceListSortFilterProxyModel::ServiceListSortFilterProxyModel(  QObject * parent )
    : QSortFilterProxyModel( parent )
{
    setSortLocaleAware( true );
    setSortCaseSensitivity( Qt::CaseInsensitive );
    setSortRole( CustomServiceRoles::SortRole );

    setDynamicSortFilter( true );
}

ServiceListSortFilterProxyModel::~ServiceListSortFilterProxyModel()
{}

bool
ServiceListSortFilterProxyModel::lessThan( const QModelIndex &left, const QModelIndex &right ) const
{
    const QVariant leftData = sourceModel()->data( left, Qt::DisplayRole );
    const QVariant rightData = sourceModel()->data( right, Qt::DisplayRole );

    const QString leftString = leftData.toString();
    const QString rightString = rightData.toString();

    //debug() << "left : " << leftString;
    //debug() << "right: " << rightString;

    return leftString.compare( rightString, Qt::CaseInsensitive ) > 0;
}

