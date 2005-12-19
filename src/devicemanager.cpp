#include <dcopobject.h>
#include <kapplication.h>
#include <dcopclient.h>
#include "medium.h"
#include <qptrlist.h>
#include "devicemanager.h"
#include "debug.h"

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
    m_valid = true;

    if (!m_dc->isRegistered())
    {
        m_valid = false;
        debug() << "DeviceManager:  DCOP Client not registered!" << endl;
    }

    if (!m_dc->connectDCOPSignal("kded", "mediamanager", "mediumAdded(QString)", "devices", "displayDevices(QString)", false) ||
        !m_dc->connectDCOPSignal("kded", "mediamanager", "mediumRemoved(QString)", "devices", "displayDevices(QString)", false) ||
        !m_dc->connectDCOPSignal("kded", "mediamanager", "mediumChanged(QString)", "devices", "displayDevices(QString)", false))
    {
        debug() << "DeviceManager:  Could not connect to signal mediumAdded!" << endl;
    }
    else
    {
        debug() << "DeviceManager:  connectDCOPSignal returned sucessfully!" << endl;
    }


}

DeviceManager::~DeviceManager()
{
}

void DeviceManager::displayDevices(QString /*name*/)
{
    DEBUG_BLOCK
    debug() << "DeviceManager: displayDevices called with name argument = " << name << endl;
    QByteArray data, replyData;
    QCString replyType;
    QDataStream arg(data, IO_WriteOnly);
    arg << 5;
    if (!m_dc->call("kded", "mediamanager", "fullList()", data, replyType, replyData))
        debug() << "Error during DCOP call" << endl;
    else
    {
        QDataStream reply(replyData, IO_ReadOnly);
        QStringList result;
        while(!reply.atEnd())
            reply >> result;
        m_currMediaList = Medium::createList( result );
        Medium::List::iterator it;
        QString mountwhere, halid;
        for ( it = m_currMediaList.begin(); it != m_currMediaList.end(); it++ )
        {
            if ( (*it).name() == name )
                //this would be where to emit the ID as a signal...?
                debug() << "ID of name argument = " << (*it).id() << endl;
            if ( (*it).mimeType().contains( "removable", false ) )
            {
                if ( (*it).isMounted() )
                {
                    mountwhere = (*it).mountPoint();
                    debug() << "Removable device " << (*it).id() << " detected, and is mounted at: " << mountwhere << endl;
                }
                else
                    debug() << "Removable device " << (*it).id() << " detected but not mounted" << endl;
            }
        }
    }
}

#include "devicemanager.moc"

