/****************************************************************************************
 * Copyright (c) 2011 Bart Cerneels <bart.cerneels@kde.org                              *
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

#ifndef OPMLDIRECTORYFILTERPROXY_H
#define OPMLDIRECTORYFILTERPROXY_H

#include <QSortFilterProxyModel>

class OpmlDirectoryFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT
    public:
        explicit OpmlDirectoryFilterProxy( QObject *parent = 0 );
        virtual ~OpmlDirectoryFilterProxy();

        /** reimplemented to enable filtering on leaf outlines only */
        virtual bool filterAcceptsRow( int source_row, const QModelIndex &source_parent ) const;

    signals:

    public slots:

};

#endif // OPMLDIRECTORYFILTERPROXY_H
