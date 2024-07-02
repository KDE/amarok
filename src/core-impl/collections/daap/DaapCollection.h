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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef DAAPCOLLECTION_H
#define DAAPCOLLECTION_H

#include "core/collections/Collection.h"
#include "MemoryCollection.h"
#include "Reader.h"

#include <QIcon>
#include <QMap>
#include <QHash>
#include <QHostInfo>
#include <QPointer>
#include <QtGlobal>
#include <QSharedPointer>

#include <KDNSSD/RemoteService>

namespace KDNSSD {
    class ServiceBrowser;
}

namespace Collections {

class DaapCollection;

class DaapCollectionFactory : public Collections::CollectionFactory
{
    Q_PLUGIN_METADATA(IID AmarokPluginFactory_iid FILE "amarok_collection-daapcollection.json")
    Q_INTERFACES(Plugins::PluginFactory)
    Q_OBJECT

    public:
        DaapCollectionFactory();
        ~DaapCollectionFactory() override;

        void init() override;

    private:
        QString serverKey( const QString& host, quint16 port ) const;

    private Q_SLOTS:
        void connectToManualServers();
        void serverOffline( KDNSSD::RemoteService::Ptr );
        void foundDaap( KDNSSD::RemoteService::Ptr );
        void resolvedDaap( bool );
        void slotCollectionReady();
        void slotCollectionDownloadFailed();

        void resolvedServiceIp(const QHostInfo&);
        void resolvedManualServerIp(const QHostInfo&);

    private:
        KDNSSD::ServiceBrowser* m_browser;

        QMap<QString, QPointer<DaapCollection> > m_collectionMap;

        QHash<int, quint16> m_lookupHash;
};

class DaapCollection : public Collections::Collection
{
    Q_OBJECT
    public:
        DaapCollection( const QString &host, const QString &ip, quint16 port );
        ~DaapCollection() override;

        QueryMaker* queryMaker() override;

        QString collectionId() const override;
        QString prettyName() const override;
        QIcon icon() const override { return QIcon::fromTheme(QStringLiteral("network-server")); }

        void serverOffline();

        QSharedPointer<MemoryCollection> memoryCollection() const { return m_mc; }

    Q_SIGNALS:
        void collectionReady();

    public Q_SLOTS:
        void loadedDataFromServer();
        void parsingFailed();

    private Q_SLOTS:
        void passwordRequired();
        void httpError( const QString &error );

    private:
        QString m_host;
        quint16 m_port;
        QString m_ip;

        Daap::Reader *m_reader;
        QSharedPointer<MemoryCollection> m_mc;
};

} //namespace Collections

#endif
