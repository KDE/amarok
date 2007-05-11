/* 
   Copyright (C) 2006 Ian Monroe <ian@monroe.nu>
   Copyright (C) 2006 Seb Ruiz <me@sebruiz.net>
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef DAAPCOLLECTION_H
#define DAAPCOLLECTION_H

#include "collection.h"
#include "memorycollection.h"
#include "reader.h"

#include <QMap>
#include <QtGlobal>

#include <dnssd/remoteservice.h> //for DNSSD::RemoteService::Ptr

using namespace Daap;

namespace DNSSD {
    class ServiceBrowser;
}

class DaapCollection;

class DaapCollectionFactory : public CollectionFactory
{
    Q_OBJECT
    public:
        DaapCollectionFactory();
        virtual ~DaapCollectionFactory();

        virtual void init();

    private:
        QString serverKey( const DNSSD::RemoteService *service ) const;

    private slots:
        void connectToManualServers();
        QString resolve( const QString &hostname );
        void serverOffline( DNSSD::RemoteService::Ptr );
        void foundDaap( DNSSD::RemoteService::Ptr );
        void resolvedDaap( bool );

    private:
        DNSSD::ServiceBrowser* m_browser;

        QMap<QString, DaapCollection*> m_collectionMap;
};

class DaapCollection : public Collection, public MemoryCollection
{
    Q_OBJECT
    public:
        DaapCollection( const QString &host, const QString &ip, quint16 port );
        virtual ~DaapCollection();

        virtual void startFullScan();
        virtual QueryMaker* queryBuilder();

        virtual QString collectionId() const;
        virtual QString prettyName() const;

        void serverOffline();

    private slots:
        void passwordRequired();
        void httpError( const QString &error );

    private:
        QString m_host;
        quint16 m_port;
        QString m_ip;

        Reader *m_reader;

};

#endif
