//
// C++ Interface: servicedynamiccollection
//
// Description: 
//
//
// Author:  <>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef SERVICEDYNAMICCOLLECTION_H
#define SERVICEDYNAMICCOLLECTION_H

#include <collection.h>

/**
A specialised collection used for services that dynamically fetch their data from somewhere ( a web service, an external program, etc....)

	@author 
*/
class ServiceDynamicCollection : public Collection
{
public:

    Q_OBJECT
    public:
        ServiceDynamicCollection( const QString &id, const QString &prettyName );
        virtual ~ServiceDynamicCollection();

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

#endif
