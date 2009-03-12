/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
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

#include "ServiceListModel.h"


#include "Debug.h"

ServiceListModel::ServiceListModel()
 : QAbstractListModel()
{
}

ServiceListModel::~ServiceListModel()
{
}

int
ServiceListModel::rowCount(const QModelIndex & parent) const
{
    Q_UNUSED( parent );
    return m_services.count();
}

QVariant
ServiceListModel::data(const QModelIndex & index, int role) const
{
    //DEBUG_BLOCK
    if( !index.isValid() || m_services.count() <= index.row() )
        return QVariant();

    switch( role )
    {
        case Qt::DisplayRole:
            return QVariant( m_services[index.row()]->translatedName() );

        case Qt::DecorationRole:
            return QVariant( m_services[index.row()]->icon() );

        case CustomServiceRoles::ShortDescriptionRole:
        case CustomServiceRoles::SortRole:
            return QVariant( m_services[index.row()]->shortDescription() );

        case CustomServiceRoles::LongDescriptionRole:
            return QVariant( m_services[index.row()]->longDescription() );

        case CustomServiceRoles::ServiceRole:
            return qVariantFromValue( m_services[index.row()] );

        case CustomServiceRoles::AlternateRowRole:
            return ( index.row() % 2 == 1 );

        default:
            return QVariant();
     }
}

void
ServiceListModel::addService(ServiceBase * service)
{
    if( !service )
    {
        debug() << "Trying to add a nonexistent service to the ServiceListModel!";
        return;
    }
    beginInsertRows ( QModelIndex(), m_services.count(), m_services.count() );
    m_services.push_back( service );
    endInsertRows();
}

void
ServiceListModel::removeService( ServiceBase * service )
{
    if( !service )
    {
        debug() << "Trying to remove a nonexistent service from the ServiceListModel!";
        return;
    }
    int index = m_services.indexOf( service );
    beginRemoveRows ( QModelIndex(), index, index );
    m_services.removeAt( index );
    endRemoveRows();
}

