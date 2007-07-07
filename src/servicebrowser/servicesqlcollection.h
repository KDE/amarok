/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *  Copyright (c) 2007 Nikolaj Hald Nielsenn <nhnFreespirit@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#ifndef AMAROK_SERVICESQLCOLLECTION_H
#define AMAROK_SERVICESQLCOLLECTION_H

#include "collection.h"
#include "servicemetabase.h"


class ServiceSqlCollection : public Collection
{
    Q_OBJECT
    public:
        ServiceSqlCollection( const QString &id, const QString &prettyName, ServiceMetaFactory * metaFactory );
        virtual ~ServiceSqlCollection();

        virtual void startFullScan() {} //TODO
        virtual QueryMaker* queryMaker();

        virtual QString collectionId() const;
        virtual QString prettyName() const;

        virtual QStringList query( const QString &query );
        virtual int insert( const QString &statement, const QString &table );

        virtual QString escape( QString text ) const;


    private:
        ServiceMetaFactory * m_metaFactory;

        QString m_collectionId;
        QString m_prettyName;
};

#endif /* AMAROK_COLLECTION_SQLCOLLECTION_H */

