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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/
 
#ifndef COLLECTIONDBUSHANDLER_H
#define COLLECTIONDBUSHANDLER_H

#include <QDBusArgument>
#include <QDBusContext>
#include <QList>
#include <QMap>
#include <QMetaType>
#include <QObject>
#include <QString>
#include <QVariant>

typedef QList<QVariantMap> VariantMapList;

class CollectionDBusHandler : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO( "Collection D-Bus Interface", "org.kde.amarok.Collection" )
    
    public:
        CollectionDBusHandler( QObject *parent );
        
    public slots:
        /*
         * Takes a query in XML form and executes it. Amarok runs queries asynchronously, therefore the result
         * of the query will be returned by the queryResult() signal. The return value of this method is a token
         * that uniquely identifies the query. It will also be the first parameter of the queryResult() signal
         * for the result of the query. Will return an empty string if the XML query is invalid.
         */
    VariantMapList Query( const QString &xmlQuery );
        
};

Q_DECLARE_METATYPE( VariantMapList )

#endif
