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

#include "IpodHandler.h"

#include "MediaDeviceCollection.h"
#include "Debug.h"

#include <KIcon>

#include <QtGlobal>

class IpodCollection;
class MediaDeviceInfo;

class IpodCollectionFactory : public MediaDeviceCollectionFactory<IpodCollection>
{
    Q_OBJECT
    public:
        IpodCollectionFactory();
        virtual ~IpodCollectionFactory();
};

class IpodCollection : public MediaDeviceCollection
{
    Q_OBJECT
    public:
        // inherited methods
        
        IpodCollection( MediaDeviceInfo* info );
        virtual ~IpodCollection();

        virtual bool possiblyContainsTrack( const KUrl &url ) const;
        virtual Meta::TrackPtr trackForUrl( const KUrl &url );



        /// TODO: commented out, will be inherited

        void copyTrackListToDevice( const Meta::TrackList tracklist );

        //virtual void deviceRemoved();

        //virtual void startFullScan();
        //virtual QueryMaker* queryMaker();

        //QString udi() const;

        virtual CollectionLocation* location() const;

        virtual QString collectionId() const;
        
        //virtual bool hasCapabilityInterface( Meta::Capability::Type type ) const;
        //virtual Meta::Capability* asCapabilityInterface( Meta::Capability::Type type );

        //virtual void collectionUpdated() { DEBUG_BLOCK emit updated(); }


        //Ipod::IpodHandler* handler() { return m_handler; }


        //virtual void updateTags( Meta::Track *track); // NOTE: TODO: forward call to handler, have it extract needed info
        void writeDatabase();
        
        virtual QString prettyName() const;
        virtual KIcon icon() const { return KIcon("multimedia-player-apple-ipod"); };

    signals:
        //void collectionReady();
        //void collectionDisconnected( const QString &udi );

        //void copyTracksCompleted( bool success );

    public slots:
        //void connectDevice();
        //void disconnectDevice();
        //void deleteTracksSlot( Meta::TrackList tracklist );

        //void slotDisconnect();

    private slots:
        //void slotCopyTracksCompleted( bool success );
        //void slotDeleteTracksCompleted();


     /** IpodCollection-specific stuff */

    public:

        //void removeTrack( const Meta::IpodTrackPtr &track );

    private:
        //Meta::IpodTrackPtr m_trackToDelete;
        QString            m_mountPoint;
        //QString            m_udi;
        //Ipod::IpodHandler *m_handler;
};
/*
class SomeDeviceCollection : public MediaDeviceCollection {

    SomeDeviceCollection();
    virtual ~SomeDeviceCollection();
}
*/
#endif
