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

#ifndef MEDIADEVICECOLLECTION_H
#define MEDIADEVICECOLLECTION_H

#include "Collection.h"
#include "MemoryCollection.h"

#include <KIcon>

#include <QtGlobal>

class MediaDeviceCollectionFactory : public Amarok::CollectionFactory
{
    Q_OBJECT
    public:
        MediaDeviceCollectionFactory();
        virtual ~MediaDeviceCollectionFactory();

        virtual void init() = 0;

    private:

    private slots:
        virtual void deviceDetected() = 0;
        virtual void deviceRemoved( const QString &udi ) = 0;
        virtual void slotCollectionReady() = 0;
        virtual void slotCollectionDisconnected( const QString & udi ) = 0;

};

class MediaDeviceCollection : public Amarok::Collection, public MemoryCollection
{
    Q_OBJECT
    public:

        void copyTrackListToDevice( const Meta::TrackList tracklist );

        virtual void startFullScan();
        virtual QueryMaker* queryMaker();

        virtual QString collectionId() const;
        virtual QString prettyName() const;
        virtual KIcon icon() const { return KIcon("drive-removable-media-usb"); }

    signals:
        void collectionReady();

    public slots:
        virtual void deleteTracksSlot( Meta::TrackList tracklist );

    protected:
        MediaDevice::MediaDeviceHandler *m_handler;
};

#endif
