/* 
   Copyright (C) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>

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

#ifndef IPODCOLLECTION_H
#define IPODCOLLECTION_H

extern "C" {
  #include <gpod/itdb.h>
}

#include "Debug.h"

#include "Collection.h"
#include "MemoryCollection.h"
#include "IpodHandler.h"

#include <QtGlobal>
#include <QMap>

class IpodCollection;

class IpodCollectionFactory : public Amarok::CollectionFactory
{
    Q_OBJECT
    public:
        IpodCollectionFactory();
        virtual ~IpodCollectionFactory();

        virtual void init();

    public slots:
        // convenience slot
        void removeIpod( const QString &udi ) { deviceRemoved( udi ); }

    private slots:
        void ipodDetected( const QString &mountPoint, const QString &udi );
        void deviceRemoved( const QString &udi );
        void slotCollectionReady();
        void slotCollectionDisconnected( const QString & udi );

    private:
        QMap<QString, IpodCollection*> m_collectionMap;
};

class IpodCollection : public Amarok::Collection, public MemoryCollection
{
    Q_OBJECT
    public:
        IpodCollection( const QString &mountPoint, const QString &udi );
        virtual ~IpodCollection();

        void copyTrackToDevice( const Meta::TrackPtr &track );
        bool deleteTrackFromDevice( const Meta::IpodTrackPtr &track );
        void removeTrack( const Meta::IpodTrackPtr &track );

        void setTrackToDelete( const Meta::IpodTrackPtr &track );

        void copyTracksCompleted();

        void deviceRemoved();

        virtual void startFullScan();
        virtual QueryMaker* queryMaker();

        QString udi() const;

        virtual CollectionLocation* location() const;

        virtual QString collectionId() const;
        virtual QString prettyName() const;

        virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const;
        virtual Meta::Capability* asCapabilityInterface( Meta::Capability::Type type );

        virtual void collectionUpdated() { DEBUG_BLOCK emit updated(); }

        Ipod::IpodHandler* handler() { return m_handler; }

        void updateTags( Meta::IpodTrack *track);
        void writeDatabase();

    signals:
        void collectionReady();
        void collectionDisconnected( const QString &udi );

    public slots:
        void connectDevice();
        void disconnectDevice();
    void deleteTrackToDelete();
    void deleteTrackSlot( Meta::IpodTrackPtr track);
    void deleteTracksSlot( Meta::TrackList tracklist );

    void slotDisconnect();

    private:
        Meta::IpodTrackPtr m_trackToDelete;
        QString            m_mountPoint;
        QString            m_udi;
        Ipod::IpodHandler *m_handler;
};

#endif
