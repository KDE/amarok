/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
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
#ifndef AMAROK_COLLECTION_SQLCOLLECTION_H
#define AMAROK_COLLECTION_SQLCOLLECTION_H

#include "collection.h"
#include "sqlregistry.h"
#include "SqlStorage.h"

class SqlCollectionFactory : public CollectionFactory
{
    Q_OBJECT

    public:
        SqlCollectionFactory() {}
        virtual ~SqlCollectionFactory() {}

        virtual void init();

    private slots:
        void testMultipleCollections();     //testing
        void removeSecondCollection();      //testing

    private:
        SqlCollection *m_secondCollection; //testing
};

class CollectionDB;

class SqlCollection : public Collection, public SqlStorage
{
    public:
        SqlCollection( const QString &id, const QString &prettyName );
        virtual ~SqlCollection();

        virtual void startFullScan() {} //TODO
        virtual QueryMaker* queryMaker();

        virtual QString collectionId() const;
        virtual QString prettyName() const;

        SqlRegistry* registry() const;

        void removeCollection();    //testing, remove later

        virtual bool possiblyContainsTrack( const KUrl &url ) const;
        virtual Meta::TrackPtr trackForUrl( const KUrl &url );

        //methods defined in SqlStorage
        virtual int sqlDatabasePriority() const;
        virtual QString type() const;

        virtual QStringList query( const QString &query );
        virtual int insert( const QString &statement, const QString &table );

        virtual QString escape( QString text ) const;

        virtual QString boolTrue() const;
        virtual QString boolFalse() const;

        virtual QString textColumnType( int length = 255 ) const;
        virtual QString exactTextColumnType( int length = 1024 ) const;
        virtual QString longTextColumnType() const;
        virtual QString randomFunc() const;

    private:
        //reuse CollectionDB until we replace it completely
        CollectionDB *m_collectionDb;

        SqlRegistry* const m_registry;

        QString m_collectionId;
        QString m_prettyName;
};

#endif /* AMAROK_COLLECTION_SQLCOLLECTION_H */

