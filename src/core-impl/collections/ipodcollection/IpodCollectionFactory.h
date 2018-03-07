/****************************************************************************************
 * Copyright (c) 2012 Matěj Laitl <matej@laitl.cz                                       *
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

#ifndef IPODCOLLECTIONFACTORY_H
#define IPODCOLLECTIONFACTORY_H

#include "core/collections/Collection.h"

#include <QMap>


namespace Solid {
class Device;
}

class IpodCollection;

class IpodCollectionFactory : public Collections::CollectionFactory
{
    Q_PLUGIN_METADATA(IID AmarokPluginFactory_iid FILE "amarok_collection-ipodcollection.json")
    Q_INTERFACES(Plugins::PluginFactory)
    Q_OBJECT

    public:
        IpodCollectionFactory();
        virtual ~IpodCollectionFactory();

        virtual void init();

    private Q_SLOTS:
        /**
         * Called when solid notifier detects a new device has been added
         */
        void slotAddSolidDevice( const QString &udi );

        /**
         * Called when solid StorageAccess device we are interested in is mounted or
         * unmounted
         */
        void slotAccessibilityChanged( bool accessible, const QString &udi );

        /**
         * Called when solid notifier detects a device has been removed
         */
        void slotRemoveSolidDevice( const QString &udi );

        /**
         * Called when "tracked" collection is destroyed
         */
        void slotCollectionDestroyed( QObject *collection );

    private:
        enum DeviceType {
            iPod, // classic wasy of accessing
            iOS // access using libimobiledevice
        };

        /**
         * Checks whether a solid device is an iPod.
         */
        bool identifySolidDevice( const QString &udi ) const;

        /**
         * Attempts to create appropriate collection for already identified solid device
         * @param udi. Should emit newCollection() if the collection was successfully
         * created and should become visible to the user.
         */
        void createCollectionForSolidDevice( const QString &udi );

        /// udi to iPod collection map
        QMap<QString, IpodCollection *> m_collectionMap;
};

#endif // IPODCOLLECTIONFACTORY_H
