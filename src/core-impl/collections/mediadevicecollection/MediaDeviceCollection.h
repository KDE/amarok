/****************************************************************************************
 * Copyright (c) 2008 Alejandro Wainzinger <aikawarazuni@gmail.com>                     *
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

#ifndef MEDIADEVICECOLLECTION_H
#define MEDIADEVICECOLLECTION_H

#include "core/collections/Collection.h"
#include "core-impl/collections/mediadevicecollection/MediaDeviceCollectionLocation.h"
#include "core-impl/collections/mediadevicecollection/support/ConnectionAssistant.h"
#include "core-impl/collections/mediadevicecollection/support/mediadevicecollection_export.h"
#include "core-impl/collections/support/MemoryCollection.h"

#include <QIcon>

#include <QtGlobal>
#include <QSharedPointer>

namespace Collections {

class MediaDeviceCollection;

/**
 * HACK: Base and Factory are separate because Q_OBJECT does not work directly with templates.
 * Templates used to reduce duplicated code in subclasses.
 */
class MEDIADEVICECOLLECTION_EXPORT MediaDeviceCollectionFactoryBase : public Collections::CollectionFactory
{
    Q_OBJECT

    public:
        ~MediaDeviceCollectionFactoryBase() override;
        void init() override;

    protected:
        MediaDeviceCollectionFactoryBase( ConnectionAssistant* assistant );

    protected Q_SLOTS:
        virtual void slotDeviceDetected( MediaDeviceInfo* info ); // detected type of device, connect it

    private Q_SLOTS:
        void slotDeviceDisconnected( const QString &udi );

    private:
        virtual MediaDeviceCollection* createCollection( MediaDeviceInfo* info ) = 0;

        ConnectionAssistant* m_assistant;
        QMap<QString, MediaDeviceCollection*> m_collectionMap;
};

template <class CollType>
class MediaDeviceCollectionFactory : public MediaDeviceCollectionFactoryBase
{
    protected:
        MediaDeviceCollectionFactory( ConnectionAssistant *assistant )
            : MediaDeviceCollectionFactoryBase( assistant )
        {}

        ~MediaDeviceCollectionFactory() override {}

    private:
        MediaDeviceCollection* createCollection( MediaDeviceInfo* info ) override
        {
            return new CollType( info );
        }
};


class MEDIADEVICECOLLECTION_EXPORT MediaDeviceCollection : public Collections::Collection
{
    Q_OBJECT

    public:
        /** Collection-related methods */

        ~MediaDeviceCollection() override;

        /**
         * url-based methods can be abstracted via use of Amarok URLs
         * subclasses simply define a protocol prefix, e.g. ipod
         */
        bool possiblyContainsTrack( const QUrl &url ) const override { Q_UNUSED(url); return false;} // TODO: NYI
        Meta::TrackPtr trackForUrl( const QUrl &url ) override { Q_UNUSED(url); return Meta::TrackPtr();  } // TODO: NYI

        QueryMaker* queryMaker() override;
        virtual void startFullScanDevice();

        // NOTE: incrementalscan and stopscan not implemented, might be used by UMS later though

        /**
            The protocol of uids coming from this collection.
            @return A string of the protocol, without the ://

            This has to be overridden for every device type, e.g. ipod://
        */
        QString uidUrlProtocol() const override { return QString(); } // TODO: NYI
        QString collectionId() const override; // uses udi

        QString prettyName() const override = 0; // NOTE: must be overridden based on device type
        QIcon icon() const override = 0; // NOTE: must be overridden based on device type

        bool hasCapacity() const override;
        float usedCapacity() const override;
        float totalCapacity() const override;

        // NOTE: location will have same method calls always, no need to redo each time
        CollectionLocation* location() override { return new MediaDeviceCollectionLocation( this ); }

        /** Capability-related methods */
        bool hasCapabilityInterface( Capabilities::Capability::Type type ) const override;
        Capabilities::Capability* createCapabilityInterface( Capabilities::Capability::Type type ) override;

        /** MediaDeviceCollection methods */
        QString udi() const { return m_udi; }

        /** MediaDeviceCollection-specific */
        Meta::MediaDeviceHandler* handler();
        void init() { m_handler->init(); } // tell handler to start connection
        void emitCollectionReady();

        virtual QAction *ejectAction() const;

        QSharedPointer<MemoryCollection> memoryCollection() const { return m_mc; }
        void collectionUpdated() { Q_EMIT updated(); }

    Q_SIGNALS:
        void collectionReady( Collections::Collection* );
        /** collectionDisconnected is called when ConnectionAssistant
          is told it is to be disconnected.  This could be
          because another part of Amarok (e.g. applet) told it to
          or because the MediaDeviceMonitor noticed it disconnect
        */
        void collectionDisconnected( const QString &udi );
        void deletingCollection();

        void attemptConnectionDone( bool success );

        void copyTracksCompleted( bool success );

    public Q_SLOTS:
        void slotAttemptConnectionDone( bool success );

        virtual void eject();
        void deleteCollection();

    protected:
        MediaDeviceCollection();

        QString m_udi;
        Meta::MediaDeviceHandler *m_handler;

        mutable QAction *m_ejectAction;

        QSharedPointer<MemoryCollection> m_mc;

};

} //namespace Collections

#endif
