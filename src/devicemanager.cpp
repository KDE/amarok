//
// C++ Implementation: devicemanager
//
// Description: Controls device/medium object handling, providing
//              helper functions for other objects
//
//
// Author: Jeff Mitchell <kde-dev@emailgoeshere.com>, (C) 2006
//         Maximilian Kossick <maximilian.kossick@googlemail.com>, (C) 2006
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
#include <dcopref.h>
#include <kapplication.h>

typedef Medium::List MediumList;
typedef QMap<QString, Medium*>::Iterator MediumIterator;

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
            if (!m_dc->call("kded", "mediamanager", "fullList()", data, replyType, replyData, false, 5000))
            {
                debug() << "During DeviceManager init, error during DCOP call" << endl;
            }
            reconcileMediumMap();
            debug() << "DeviceManager:  connectDCOPSignal returned successfully!" << endl;
        }
    }
}

DeviceManager::~DeviceManager()
{
    for( MediumIterator it = m_mediumMap.begin(); it != m_mediumMap.end(); it++ )
        delete (*it);
}

void
DeviceManager::mediumAdded( const QString name )
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


void
DeviceManager::mediumRemoved( const QString name )
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
    {
        delete removedMedium;   //If we are to remove it from the map, delete it first
        m_mediumMap.remove(name);
    }
}


void
DeviceManager::mediumChanged( const QString name )
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
Medium::List
DeviceManager::getDeviceList()
{
    return Medium::createList( getDeviceStringList() );
}

QStringList
DeviceManager::getDeviceStringList()
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

    return result;
}

Medium*
DeviceManager::getDevice( const QString name )
{
    DEBUG_BLOCK
    if ( !m_valid )
        return 0;
    debug() << "DeviceManager: getDevice called with name argument = " << name << endl;
    Medium* returnedMedium = 0;
    MediumList currMediumList = getDeviceList();

    for ( Medium::List::iterator it = currMediumList.begin(); it != currMediumList.end(); ++it )
    {
        if ( (*it).name() == name )
        {
            MediumIterator secIt;
            if ( (secIt = m_mediumMap.find( name )) != m_mediumMap.end() )
            {
                //Refresh the Medium by reconstructing then copying it over.
                returnedMedium = *secIt;
                *returnedMedium = Medium( *it );
            }
            else
            {
                //No previous version of this Medium - create it
                returnedMedium = new Medium( *it );
                m_mediumMap[ name ] = returnedMedium;
            }
            break;
        }
    }
    return returnedMedium;
}

void
DeviceManager::reconcileMediumMap()
{
    DEBUG_BLOCK
    if ( !m_valid )
        return;

    MediumList currMediumList = getDeviceList();

    Medium::List::iterator it;
    for ( it = currMediumList.begin(); it != currMediumList.end(); ++it )
    {
        MediumIterator locIt;
        if ( (locIt = m_mediumMap.find( (*it).name() )) != m_mediumMap.end() )
        {
            Medium* mediumHolder = (*locIt);
            *mediumHolder = Medium( *it );
        }
        else
            m_mediumMap[ (*it).name() ] = new Medium(*it);
    }

    //Sanity check
    if ( currMediumList.size() != m_mediumMap.size() )
        warning() << "Number of devices does not equal expected number" << endl;
}

QString DeviceManager::convertMediaUrlToDevice( QString url )
{
    QString device;
    if ( url.startsWith( "media:" ) || url.startsWith( "system:" ) )
    {
        KURL devicePath( url );
        DCOPRef mediamanager( "kded", "mediamanager" );
        DCOPReply reply = mediamanager.call( "properties(QString)", devicePath.fileName() );
        if ( reply.isValid() ) {
            QStringList properties = reply;
            device = properties[ 5 ];
            //kdDebug() << "DeviceManager::convertMediaUrlToDevice() munged to: " << device << "\n";
        } else
            device = QString();
    }
    else
        device = url;

    return device;
}

#include "devicemanager.moc"
