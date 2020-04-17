/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
 * Copyright (c) 2007 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#ifndef AMAROK_SERVICESQLCOLLECTION_H
#define AMAROK_SERVICESQLCOLLECTION_H

#include "amarok_export.h"
#include "ServiceCollection.h"
#include "ServiceMetaBase.h"
#include "ServiceSqlRegistry.h"

namespace Collections {

class AMAROK_EXPORT ServiceSqlCollection : public ServiceCollection
{
    Q_OBJECT
    public:
        ServiceSqlCollection( const QString &id, const QString &prettyName, ServiceMetaFactory * metaFactory, ServiceSqlRegistry * registry );
        ~ServiceSqlCollection() override;

        virtual void startFullScan() {} //TODO
        QueryMaker* queryMaker() override;

        QString collectionId() const override;
        QString prettyName() const override;

        QStringList query( const QString &query ) override;
        int insert( const QString &statement, const QString &table ) override;

        QString escape( const QString &text ) const override;

        void emitUpdated() { Q_EMIT( updated() ); }

        Meta::TrackPtr trackForUrl( const QUrl &url ) override;
        bool possiblyContainsTrack( const QUrl &url ) const override;


    private:
        ServiceMetaFactory * m_metaFactory;
        ServiceSqlRegistry * m_registry;

        QString m_collectionId;
        QString m_prettyName;
};

} //namespace Collections

#endif /* AMAROK_COLLECTION_SQLCOLLECTION_H */

