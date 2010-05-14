/****************************************************************************************
 * Copyright (c) 2010 Nikhil Marathe <nsm.nikhil@gmail.com>                             *
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

#ifndef UPNPCOLLECTION_H
#define UPNPCOLLECTION_H

#include "core/collections/Collection.h"
#include "MemoryCollection.h"

#include <QMap>
#include <QHash>
#include <QHostInfo>
#include <QPointer>
#include <QtGlobal>
#include <QSharedPointer>

#include <KIcon>

namespace Herqq {
  namespace Upnp {
    class HControlPoint;
    class HDevice;
  }
}

namespace Collections {

class UpnpCollection;

class UpnpCollectionFactory : public Collections::CollectionFactory
{
    Q_OBJECT
    public:
        UpnpCollectionFactory( QObject *parent, const QVariantList &args );
        virtual ~UpnpCollectionFactory();

        void init();


    private:

    private slots:
        void rootDeviceOnline(Herqq::Upnp::HDevice *device);

    private:
        Herqq::Upnp::HControlPoint *m_controlPoint;
};

class UpnpCollection : public Collections::Collection
{
    Q_OBJECT
    public:
        UpnpCollection();
        virtual ~UpnpCollection();

        virtual void startFullScan();
        virtual QueryMaker* queryMaker();

        virtual QString collectionId() const;
        virtual QString prettyName() const;
        virtual KIcon icon() const { return KIcon("network-server"); }

        QSharedPointer<MemoryCollection> memoryCollection() const { return m_mc; }

        bool possiblyContainsTrack( const KUrl &url ) const;
    signals:

    public slots:

    private slots:

    private:
        QSharedPointer<MemoryCollection> m_mc;
};

} //namespace Collections

#endif
