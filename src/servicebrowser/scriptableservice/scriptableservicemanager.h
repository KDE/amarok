#ifndef AMAROKSCRIPTABLESERVICEMANAGER_H
#define AMAROKSCRIPTABLESERVICEMANAGER_H

#include <QObject>
#include <QString>

#include "../servicebase.h"
#include "scriptableservice.h"
#include "scriptableservicecontentmodel.h"
 
class ScriptableServiceManager : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.amarok.ScriptableServiceManager")

    public:
        ScriptableServiceManager(QObject* parent);

    Q_SIGNALS:
        void addService( ServiceBase * service );

    public Q_SLOTS:
        Q_SCRIPTABLE bool createService( QString name, QString listHeader);
        Q_SCRIPTABLE int insertElement( QString name, QString url, QString infoHtml, int parentId, QString serviceName);
        Q_SCRIPTABLE void updateComplete( );

    private:

    QMap<QString, ScriptableService *> m_serviceMap;
};
#endif
