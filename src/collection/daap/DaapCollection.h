/****************************************************************************************
 * Copyright (c) 2006 Ian Monroe <ian@monroe.nu>                                        *
 * Copyright (c) 2006 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef DAAPCOLLECTION_H
#define DAAPCOLLECTION_H

#include "Collection.h"
#include "MemoryCollection.h"
#include "Reader.h"

#include <QMap>
#include <QHash>
#include <QHostInfo>
#include <QPointer>
#include <QtGlobal>

#include <KIcon>

#include <dnssd/remoteservice.h> //for DNSSD::RemoteService::Ptr

namespace DNSSD {
    class ServiceBrowser;
}

class DaapCollection;

class DaapCollectionFactory : public Amarok::CollectionFactory
{
    Q_OBJECT
    public:
        DaapCollectionFactory();
        virtual ~DaapCollectionFactory();

        virtual void init();

    private:
        QString serverKey( const QString& host, quint16 port ) const;

    private slots:
        void connectToManualServers();
        void serverOffline( DNSSD::RemoteService::Ptr );
        void foundDaap( DNSSD::RemoteService::Ptr );
        void resolvedDaap( bool );
        void slotCollectionReady();
        void slotCollectionDownloadFailed();

        void resolvedServiceIp(QHostInfo);
        void resolvedManualServerIp(QHostInfo);

    private:
        DNSSD::ServiceBrowser* m_browser;

        QMap<QString, QPointer<DaapCollection> > m_collectionMap;

        QHash<int, quint16> m_lookupHash;
};

class DaapCollection : public Amarok::Collection, public MemoryCollection
{
    Q_OBJECT
    public:
        DaapCollection( const QString &host, const QString &ip, quint16 port );
        virtual ~DaapCollection();

        virtual void startFullScan();
        virtual QueryMaker* queryMaker();

        virtual QString collectionId() const;
        virtual QString prettyName() const;
        virtual KIcon icon() const { return KIcon("network-server"); }

        void serverOffline();

    signals:
        void collectionReady();

    public slots:
        void loadedDataFromServer();
        void parsingFailed();

    private slots:
        void passwordRequired();
        void httpError( const QString &error );

    private:
        QString m_host;
        quint16 m_port;
        QString m_ip;

        Daap::Reader *m_reader;
};

#endif
