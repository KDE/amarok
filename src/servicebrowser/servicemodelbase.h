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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#ifndef AMAROKSERVICEMODELBASEL_H
#define AMAROKSERVICEMODELBASEL_H

#include <QAbstractItemModel>

class ServiceModelBase : public QAbstractItemModel
{
    Q_OBJECT

public:
    ServiceModelBase( QObject *parent );
    virtual ~ServiceModelBase() {}
    virtual void requestHtmlInfo ( const QModelIndex & item ) const = 0;
    void resetModel();
    Qt::ItemFlags flags ( const QModelIndex & index ) const;
    virtual QMimeData* mimeData( const QModelIndexList &indices ) const;

signals:

    void infoChanged( QString infoHtml ) const;

};

 #endif
