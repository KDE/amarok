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

#ifndef SERVICELISTMODEL_H
#define SERVICELISTMODEL_H

#include "services/ServiceBase.h"

#include <QAbstractListModel>

#include <QList>

Q_DECLARE_METATYPE(ServiceBase *)

namespace CustomServiceRoles
{
    enum CustomServiceRolesId {
        ShortDescriptionRole = Qt::UserRole + 1,
        LongDescriptionRole = Qt::UserRole + 2,
        ServiceRole = Qt::UserRole + 3,
        AlternateRowRole = Qt::UserRole + 4,
        SortRole = Qt::UserRole + 5
    };
}

/**
A very simple model to hold the available services

    @author
*/
class ServiceListModel : public QAbstractListModel
{
public:

    ServiceListModel ();
    ~ServiceListModel();

    int rowCount( const QModelIndex & parent = QModelIndex() ) const;
    QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;

    void addService( ServiceBase * service );
    void removeService(  ServiceBase * service );

private:
    QList<ServiceBase * > m_services;
};

#endif
