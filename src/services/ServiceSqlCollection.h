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


class AMAROK_EXPORT ServiceSqlCollection : public ServiceCollection
{
    Q_OBJECT
    public:
        ServiceSqlCollection( const QString &id, const QString &prettyName, ServiceMetaFactory * metaFactory, ServiceSqlRegistry * registry );
        virtual ~ServiceSqlCollection();

        virtual void startFullScan() {} //TODO
        virtual QueryMaker* queryMaker();

        virtual QString collectionId() const;
        virtual QString prettyName() const;

        virtual QStringList query( const QString &query );
        virtual int insert( const QString &statement, const QString &table );

        virtual QString escape( QString text ) const;

        void emitUpdated() { emit( updated() ); }

        virtual Meta::TrackPtr trackForUrl( const KUrl &url );
        virtual bool possiblyContainsTrack( const KUrl &url ) const;


    private:
        ServiceMetaFactory * m_metaFactory;
        ServiceSqlRegistry * m_registry;

        QString m_collectionId;
        QString m_prettyName;
};

#endif /* AMAROK_COLLECTION_SQLCOLLECTION_H */

