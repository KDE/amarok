//
// C++ Implementation: mediadevicemanager
//
// Description: Controls device/medium object handling, providing
//              helper functions for other objects
//
//
// Author: Jeff Mitchell <kde-dev@emailgoeshere.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "amarok.h"
#include "amarokconfig.h"
#include "debug.h"
#include "devicemanager.h"
#include "mediadevicemanager.h"
#include "medium.h"

#include <qptrlist.h>
#include <qtimer.h>

#include <dcopclient.h>
#include <dcopobject.h>
#include <kapplication.h>

typedef Medium::List MediumList;

MediaDeviceManager* MediaDeviceManager::instance()
{
    static MediaDeviceManager dw;
    return &dw;
}

MediaDeviceManager::MediaDeviceManager()
{
    DEBUG_BLOCK
    connect( DeviceManager::instance(), SIGNAL( mediumAdded( const Medium*, QString ) ), SLOT( slotMediumAdded( const Medium*, QString ) ) );
    connect( DeviceManager::instance(), SIGNAL( mediumChanged( const Medium*, QString ) ), SLOT( slotMediumChanged( const Medium*, QString ) ) );
    connect( DeviceManager::instance(), SIGNAL( mediumRemoved( const Medium*, QString ) ), SLOT( slotMediumRemoved( const Medium*, QString ) ) );
    Medium::List mediums = DeviceManager::instance()->getDeviceList();
    foreachType( Medium::List, mediums )
    {
        slotMediumAdded( &(*it), (*it).id() );
    }
    if( !mediums.count() )
    {
        debug() << "DeviceManager didn't return any devices, we are probably running on a non-KDE system. Trying to reinit media devices later" << endl;
        QTimer::singleShot( 4000, this, SLOT( reinitDevices() ) );
    }
    //load manual devices
    QStringList manualDevices;
    KConfig *config = Amarok::config( "MediaBrowser" );
    QMap<QString,QString> savedDevices = config->entryMap( "MediaBrowser" );
    QMap<QString,QString>::Iterator qit;
    QString curr, currMountPoint, currName;
    for( qit = savedDevices.begin(); qit != savedDevices.end(); ++qit )
    {
        //only handle manual devices, autodetected devices should be added on the fly
        if( qit.key().startsWith( "manual|" ) )
        {
            curr = qit.key();
            curr = curr.remove( "manual|" );
            currName = curr.left( curr.find( '|' ) );
            currMountPoint = curr.remove( currName + '|' );
            manualDevices.append( "false" );           //autodetected
            manualDevices.append( qit.key() );          //id
            manualDevices.append( currName );          //name
            manualDevices.append( currName );          //label
            manualDevices.append( QString::null );     //userLabel
            manualDevices.append( "unknown" );         //mountable?
            manualDevices.append( QString::null );     //device node
            manualDevices.append( currMountPoint );    //mountPoint
            manualDevices.append( "manual" );          //fsType
            manualDevices.append( "unknown" );         //mounted
            manualDevices.append( QString::null );     //baseURL
            manualDevices.append( QString::null );     //MIMEtype
            manualDevices.append( QString::null );     //iconName
            manualDevices.append( "---" );             //separator
        }
    }
    Medium::List manualMediums = Medium::createList( manualDevices );
    foreachType( Medium::List, manualMediums )
    {
        slotMediumAdded( &(*it), (*it).id() );
    }
}

MediaDeviceManager::~MediaDeviceManager()
{
}

void
MediaDeviceManager::addManualDevice( Medium* added )
{
    m_mediumMap[added->name()] = added;
    added->setFsType( "manual" );
    emit mediumAdded( added, added->name() );
}

void
MediaDeviceManager::removeManualDevice( Medium* removed )
{
    emit mediumRemoved( removed, removed->name() );
    if( m_mediumMap.contains( removed->name() ) )
        m_mediumMap.remove( removed->name() );
}

void MediaDeviceManager::slotMediumAdded( const Medium *m, QString id)
{
    DEBUG_BLOCK
    if ( m )
    {
        if ( m->fsType() == "manual" ||
                ( !m->deviceNode().startsWith( "/dev/hd" ) &&
                  (m->fsType() == "vfat" || m->fsType() == "hfsplus" || m->fsType() == "msdosfs" ) ) )
            // add other fsTypes that should be auto-detected here later
        {
            if ( m_mediumMap.contains( m->name() ) )
            {
                Medium *tempMedium = m_mediumMap[m->name()];
                m_mediumMap.remove( m->name() );
                delete tempMedium;
            }
            m_mediumMap[m->name()] = new Medium( m );
            emit mediumAdded( m, id );
        }
    }
}

void MediaDeviceManager::slotMediumChanged( const Medium *m, QString id )
{
    //nothing to do here
    emit mediumChanged( m, id);
}

void MediaDeviceManager::slotMediumRemoved( const Medium* , QString id )
{
    DEBUG_BLOCK
    Medium* removedMedium = 0;
    if ( m_mediumMap.contains(id) )
        removedMedium = m_mediumMap[id];
    if ( removedMedium )
        debug() << "[MediaDeviceManager::slotMediumRemoved] Obtained medium name is " << id << ", id is: " << removedMedium->id() << endl;
    else
        debug() << "[MediaDeviceManager::slotMediumRemoved] Medium was unknown and is null; name was " << id << endl;
    //if you get a null pointer from this signal, it means we did not know about the device
    //before it was removed, i.e. the removal was the first event for the device received while amarok
    //has been running
    //There is no point in calling getDevice, since it will not be in the list anyways
    emit mediumRemoved( removedMedium, id );
    if ( m_mediumMap.contains(id) )
        m_mediumMap.remove(id);
    delete removedMedium;
}

Medium* MediaDeviceManager::getDevice( QString name )
{
    return DeviceManager::instance()->getDevice( name );
}

void MediaDeviceManager::reinitDevices( )
{
    Medium::List mediums = DeviceManager::instance()->getDeviceList();
    foreachType( Medium::List, mediums )
    {
        slotMediumAdded( &(*it), (*it).id() );
    }
}

#include "mediadevicemanager.moc"
