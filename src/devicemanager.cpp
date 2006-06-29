//
// C++ Implementation: devicemanager
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
#include "medium.h"
#include "mediumpluginmanager.h"

#include <qptrlist.h>
#include <qtimer.h>

#include <dcopclient.h>
#include <dcopobject.h>
#include <kapplication.h>

typedef Medium::List MediumList;

DeviceManager* DeviceManager::instance()
{
    static DeviceManager dw;
    return &dw;
}

DeviceManager::DeviceManager()
{
    DEBUG_BLOCK
    m_dc = KApplication::dcopClient();
    m_dc->setNotifications(true);
    m_valid = false;

    if (!m_dc->isRegistered())
    {
        debug() << "DeviceManager:  DCOP Client not registered!" << endl;
    }
    else
    {
        if (!m_dc->connectDCOPSignal("kded", "mediamanager", "mediumAdded(QString)", "devices", "mediumAdded(QString)", false) ||
            !m_dc->connectDCOPSignal("kded", "mediamanager", "mediumRemoved(QString)", "devices", "mediumRemoved(QString)", false) ||
            !m_dc->connectDCOPSignal("kded", "mediamanager", "mediumChanged(QString)", "devices", "mediumChanged(QString)", false))
        {
            debug() << "DeviceManager:  Could not connect to signal mediumAdded/Removed/Changed!" << endl;
        }
        else
        {
            m_valid = true;
            
            //run the DCOP query here because apparently if you don't run KDE as a DM the first call will fail
            //...go figure
            QByteArray data, replyData;
            QCString replyType;
            QDataStream arg(data, IO_WriteOnly);
            QStringList result;
            arg << 5;
            if (!m_dc->call("kded", "mediamanager", "fullList()", data, replyType, replyData))
            {
                debug() << "During DeviceManager init, error during DCOP call" << endl;
            }
                                
            getDevice( "init" );
            QTimer::singleShot(4000, this, SLOT( reinitDevices() ) ); 
            debug() << "DeviceManager:  connectDCOPSignal returned sucessfully!" << endl;
        }
    }
}

DeviceManager::~DeviceManager()
{
}

void DeviceManager::reinitDevices() //SLOT
{
    MediumPluginManager *mpm = new MediumPluginManager( 0, true );
    mpm->detectDevices( true, true );
    mpm->finished();
    delete mpm;
}

void DeviceManager::mediumAdded( QString name )
{
    DEBUG_BLOCK
    if ( !m_valid )
        return;
    Medium* addedMedium = getDevice(name);
    if ( addedMedium != 0 )
        debug() << "[DeviceManager::mediumAdded] Obtained medium name is " << name << ", id is: " << addedMedium->id() << endl;
    else
        debug() << "[DeviceManager::mediumAdded] Obtained medium is null; name was " << name << endl;
    emit mediumAdded( addedMedium, name );
}


void DeviceManager::mediumRemoved( QString name )
{
    DEBUG_BLOCK
    if ( !m_valid )
        return;
    Medium* removedMedium = 0;
    if ( m_mediumMap.contains(name) )
        removedMedium = m_mediumMap[name];
    if ( removedMedium != 0 )
        debug() << "[DeviceManager::mediumRemoved] Obtained medium name is " << name << ", id is: " << removedMedium->id() << endl;
    else
        debug() << "[DeviceManager::mediumRemoved] Medium was unknown and is null; name was " << name << endl;
    //if you get a null pointer from this signal, it means we did not know about the device
    //before it was removed, i.e. the removal was the first event for the device received while amarok
    //has been running
    //There is no point in calling getDevice, since it will not be in the list anyways
    emit mediumRemoved( removedMedium, name );
    if ( m_mediumMap.contains(name) )
        m_mediumMap.remove(name);
    delete removedMedium;
}


void DeviceManager::mediumChanged( QString name )
{
    DEBUG_BLOCK
    if ( !m_valid )
        return;
    Medium *changedMedium = getDevice(name);
    if ( changedMedium != 0 )
        debug() << "[DeviceManager::mediumChanged] Obtained medium name is " << name << ", id is: " << changedMedium->id() << endl;
    else
        debug() << "[DeviceManager::mediumChanged] Obtained medium is null; name was " << name << endl;
    emit mediumChanged( changedMedium, name );
}


/*
BIG FAT WARNING:
Values returned from the below function should not be counted on being unique!
For instance, there may be a Medium object in the QMap that can be accessed through
other functions that has the same data as the Medium object returned, but is a different object.
As you should not be writing to this object, this is okay, however:

Use the Medium's name or id, not the pointer value, for equality comparison!!!

This function does rebuild the map every time it is called, however this should be rare enough
that it is not a problem.
*/
Medium::List DeviceManager::getDeviceList( bool autoonly )
{
    return Medium::createList( getDeviceStringList( autoonly ) );
}

QStringList
DeviceManager::getDeviceStringList( bool autoonly )
{
    DEBUG_BLOCK
    MediumList currMediumList;

    if ( !m_valid )
    {
        QStringList blah;
        return blah;
    }

    //normal kded Medium doesn't have autodetect, so decrease by 1
    int autodetect_insert = Medium::PROPERTIES_COUNT - 1;

    QByteArray data, replyData;
    QCString replyType;
    QDataStream arg(data, IO_WriteOnly);
    QStringList result;
    arg << 5;
    if (!m_dc->call("kded", "mediamanager", "fullList()", data, replyType, replyData))
    {
        debug() << "Error during DCOP call" << endl;
    }
    else
    {
        QDataStream reply(replyData, IO_ReadOnly);
        while(!reply.atEnd())
        {
            reply >> result;
        }
        QStringList::Iterator it;
        for( it = result.begin(); it != result.end(); ++it )
        {
            if (autodetect_insert == Medium::PROPERTIES_COUNT - 1)
                result.insert(it, QString("true"));
            autodetect_insert--;
            if (autodetect_insert == -1)
                autodetect_insert = Medium::PROPERTIES_COUNT - 1;
        }
    }
    
    if( autoonly )
        return result;

    KConfig *config = amaroK::config( "MediaBrowser" );
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
            result.append( "false" );           //autodetected
            result.append( qit.key() );          //id
            result.append( currName );          //name
            result.append( currName );          //label
            result.append( QString::null );     //userLabel
            result.append( "unknown" );         //mountable?
            result.append( QString::null );     //device node
            result.append( currMountPoint );    //mountPoint
            result.append( "manual" );          //fsType
            result.append( "unknown" );         //mounted
            result.append( QString::null );     //baseURL
            result.append( QString::null );     //MIMEtype
            result.append( QString::null );     //iconName
            result.append( "---" );             //separator
        }
    }

    return result;
}

Medium* DeviceManager::getDevice( QString name )
{
    DEBUG_BLOCK
    if ( !m_valid )
        return NULL;
    debug() << "DeviceManager: getDevice called with name argument = " << name << endl;
    Medium* returnedMedium = 0;
    Medium* tempMedium = 0;
    MediumList currMediumList = getDeviceList();

    Medium::List::iterator it;
    QString mountwhere, halid;
    for ( it = currMediumList.begin(); it != currMediumList.end(); it++ )
    {
        if ( (*it).fsType() != "vfat" &&
             (*it).fsType() != "hfsplus" &&
             (*it).fsType() != "manual" ) //&& other supported fsTypes here later
            continue;
        if ( (*it).name() == name )
        {
            returnedMedium = new Medium(*it);
        }
        if( m_mediumMap.contains( name ) )
        {
            tempMedium = m_mediumMap[(*it).name()];
            m_mediumMap.remove( (*it).name() );
            delete tempMedium;
        }
        m_mediumMap[(*it).name()] = new Medium(*it);
    }

    return returnedMedium;
}

void
DeviceManager::addManualDevice( Medium* added )
{
    m_mediumMap[added->name()] = added;
    added->setFsType( "manual" );
    emit mediumAdded( added, added->name() );
}

void
DeviceManager::removeManualDevice( Medium* removed )
{
    emit mediumRemoved( removed, removed->name() );
    if( m_mediumMap.contains( removed->name() ) )
        m_mediumMap.remove( removed->name() );
}

#include "devicemanager.moc"
