#include "AudioCdCollectionFactory.h"
#include "AudioCdCollection.h"

#include "core/support/Debug.h"

#include <solid/block.h>
#include <solid/deviceinterface.h>
#include <solid/devicenotifier.h>
#include <solid/device.h>
#include <solid/opticaldrive.h>

AMAROK_EXPORT_COLLECTION( AudioCdCollectionFactory, audiocdcollection )

AudioCdCollectionFactory::AudioCdCollectionFactory( QObject *parent, const QVariantList &args )
    : CollectionFactory( parent, args )
{
    m_info = KPluginInfo( "amarok_collection-audiocdcollection.desktop", "services" );
}

AudioCdCollectionFactory::~AudioCdCollectionFactory()
{
}

void
AudioCdCollectionFactory::init()
{
    connect( Solid::DeviceNotifier::instance(), SIGNAL(deviceAdded(QString)),
             SLOT(slotAddSolidDevice(QString)) );
    connect( Solid::DeviceNotifier::instance(), SIGNAL(deviceRemoved(QString)),
             SLOT(slotRemoveSolidDevice(QString)) );

    // detect AudioCd devices that were already connected on startup
    QList<Solid::Device> devices = Solid::Device::listFromType( Solid::DeviceInterface::OpticalDisc );
    foreach( const Solid::Device &device, devices )
    {
        if( identifySolidDevice( device.udi() ) )
            createCollectionForSolidDevice( device.udi() );
    }
    m_initialized = true;
}

void
AudioCdCollectionFactory::slotAddSolidDevice( const QString &udi )
{
    DEBUG_BLOCK

    if( m_collectionMap.contains( udi ) )
        return; // a device added twice (?)

    if( identifySolidDevice( udi ) )
        createCollectionForSolidDevice( udi );
}

void
AudioCdCollectionFactory::slotRemoveSolidDevice( const QString &udi )
{
    AudioCdCollection *collection = m_collectionMap.take( udi );
    if( collection )
        collection->slotDestroy();
}

void
AudioCdCollectionFactory::slotRemoveAndTeardownSolidDevice( const QString &udi )
{
    AudioCdCollection *collection = m_collectionMap.take( udi );
    if( collection )
        collection->slotEject();
}

void
AudioCdCollectionFactory::slotCollectionDestroyed( QObject *collection )
{
    // remove destroyed collection from m_collectionMap
    QMutableMapIterator<QString, AudioCdCollection *> it( m_collectionMap );
    while( it.hasNext() )
    {
        it.next();
        if( (QObject *) it.value() == collection )
            it.remove();
    }
}

bool
AudioCdCollectionFactory::identifySolidDevice( const QString &udi ) const
{
    Solid::Device device( udi );
    if( device.is<Solid::OpticalDisc>() )
    {
        debug() << "OpticalDisc was found";
        const Solid::OpticalDisc * opt = device.as<Solid::OpticalDisc>();
        if ( opt->availableContent() & Solid::OpticalDisc::Audio )
        {
            debug() << "AudioCd";
            return true;
        }
    }
    return false;
}

void
AudioCdCollectionFactory::slotCollectionLoaded( bool succesfull, AudioCdCollection* collection )
{
    if( !collection )
        return;
    if( succesfull )
        emit newCollection( collection );
    else
        collection->deleteLater();
}

void
AudioCdCollectionFactory::createCollectionForSolidDevice( const QString &udi )
{
    DEBUG_BLOCK
    Solid::Device device( udi );

    AudioCdCollection *collection = new AudioCdCollection( udi );
    m_collectionMap.insert( udi, collection );

    connect( collection, SIGNAL(loaded(bool, AudioCdCollection*)),
             this, SLOT(slotCollectionLoaded(bool, AudioCdCollection*)));
    // when the collection is destroyed by someone else, remove it from m_collectionMap:
    connect( collection, SIGNAL(destroyed(QObject*)), SLOT(slotCollectionDestroyed(QObject*)) );
}
