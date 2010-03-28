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
 
#ifndef DBUSQUERYHELPER_H
#define DBUSQUERYHELPER_H

#include "core/meta/Meta.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>
#include <QVariant>

namespace Collections {
    class QueryMaker;
}

typedef QList<QVariantMap> VariantMapList;

class DBusQueryHelper : public QObject
{
    Q_OBJECT
    
    public:
        DBusQueryHelper( QObject *parent, Collections::QueryMaker *qm, const QDBusConnection &conn, const QDBusMessage &msg, bool mprisCompatible );
        
    private slots:
        void slotResultReady( const QString &collectionId, const Meta::TrackList &tracks );
        
        void slotQueryDone();

        void abortQuery();
        
    private:
        QDBusConnection m_connection;
        QDBusMessage m_message;
        VariantMapList m_result;
        bool m_mprisCompatibleResult;
        bool m_timeout;
};

#endif
