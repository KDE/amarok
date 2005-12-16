#include <dcopobject.h>
#include <kapplication.h>
#include <dcopclient.h>
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

void DeviceManager::displayDevices(QString name)
{
    DEBUG_BLOCK
    QByteArray data, replyData;
    QCString replyType;
    QDataStream arg(data, IO_WriteOnly);
    arg << 5;
    if (!m_dc->call("kded", "mediamanager", "fullList()", data, replyType, replyData))
        debug() << "Error during DCOP call" << endl;
    else
    {
        QDataStream reply(replyData, IO_ReadOnly);
        debug() << "replyType == " << replyType << endl;
        if (replyType == "QStringList")
        {
            QStringList result;
            while(!reply.atEnd())
                reply >> result;
            debug() << "The result is " <<  result << endl;
        }
        else
            debug() << "unexpected type of reply from dcop call" << endl;
    }

    //DCOPRef mediamanager("kded", "mediamanager");
    //DCOPReply reply = mediamanager.call("fullList", 5);
    //if (!reply.isValid())
    //    debug() << "Error during DCOP call" << endl;
    //else
    //    debug() << "The result is: " << reply << endl;
}

#include "devicemanager.moc"

