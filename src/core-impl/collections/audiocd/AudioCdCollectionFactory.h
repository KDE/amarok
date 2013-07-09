#ifndef AUDIOCDCOLLECTIONFACTORY_H
#define AUDIOCDCOLLECTIONFACTORY_H

#include "core/collections/Collection.h"

using namespace Collections;

class AudioCdCollection;
class AudioCdCollectionFactory : public CollectionFactory
{
    Q_OBJECT

    public:
        AudioCdCollectionFactory( QObject *parent, const QVariantList &args );
        virtual ~AudioCdCollectionFactory();

        virtual void init();

    private slots:
        /**
         * Called when solid notifier detects a new device has been added
         */
        void slotAddSolidDevice( const QString &udi );

        /**
         * Called when solid notifier detects a device has been removed
         */
        void slotRemoveSolidDevice( const QString &udi );

        /**
         * Like @see slotRemoveSolidDevice(), but instructs Collection to eject the
         * device after it has performed necessary teardown operations.
         *
         * Called when user wants to unmount the device from for example Device Notifier
         */
        void slotRemoveAndTeardownSolidDevice( const QString &udi );

        /**
         * Called when tracked collection is loaded
         */
        void slotCollectionLoaded( bool succesfull, AudioCdCollection* collection );
        /**
         * Called when "tracked" collection is destroyed
         */
        void slotCollectionDestroyed( QObject *collection );

    private:
        /**
         * Checks whether a solid device is a CD
         */
        bool identifySolidDevice( const QString &udi ) const;

        /**
         * Attempts to create appropriate collection for already identified solid device
         * @param udi. Should emit newCollection() if the collection was successfully
         * created and should become visible to the user.
         */
        void createCollectionForSolidDevice( const QString &udi );

        // maps device udi to active AudioCd collections
        QMap<QString, AudioCdCollection* > m_collectionMap;
};
#endif
