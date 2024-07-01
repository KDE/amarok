/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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
 
#ifndef COLLECTIONDBUSHANDLER_H
#define COLLECTIONDBUSHANDLER_H

#include <QDBusContext>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QVariant>

typedef QList<QVariantMap> VariantMapList;

class CollectionDBusHandler : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO( "Collection D-Bus Interface", "org.kde.amarok.Collection" )
    
    public:
        explicit CollectionDBusHandler( QObject *parent );
        
    public Q_SLOTS:
        /*
         * Takes a query in XML form and executes it. Will return an empty map if the query XML is invalid
         *
         * This method return the metadata in a format that is not compatible to MPRIS. It is therefore superceded
         * by MprisQuery and will be removed for Amarok 2.3
         */
    VariantMapList Query( const QString &xmlQuery );

        /*
         * Takes a query in XML form and executes it. Will return an empty map if the query XML is invalid
         * Returns the metadata in a MPRIS compatible format
         *
         */
    VariantMapList MprisQuery( const QString &xmlQuery );
        
};

Q_DECLARE_METATYPE( VariantMapList )

#endif
