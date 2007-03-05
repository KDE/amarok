#ifndef AMAROKSCRIPTABLESERVICEMANAGER_H
#define AMAROKSCRIPTABLESERVICEMANAGER_H

#include <QObject>
#include <QString>

#include "../servicebase.h"
 
class ScriptableServiceManager : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.amarok.ScriptableServiceManager")

    public:
        ScriptableServiceManager(QObject* parent);

    Q_SIGNALS:
        void addService( ServiceBase * service );

    public Q_SLOTS:
        Q_SCRIPTABLE bool createService( QString name );
};
#endif
