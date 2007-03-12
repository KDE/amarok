/*
Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public License
along with this library; see the file COPYING.LIB.  If not, write to
the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
Boston, MA 02110-1301, USA.
*/

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

        Q_SCRIPTABLE bool createService( const QString &name, const QString &listHeader, const QString &rootHtml);
        Q_SCRIPTABLE int insertElement( const QString &name, const QString &url, const QString &infoHtml, int parentId, const QString &serviceName);
        Q_SCRIPTABLE int insertDynamicElement( const QString &name, const QString &callbackScript, 
                                               const QString &callbackArgument, const QString &infoHtml, 
                                               int parentId, const QString &serviceName);
        Q_SCRIPTABLE bool updateComplete( const QString &serviceName );

    private:

    QMap<QString, ScriptableService *> m_serviceMap;
    QString m_rootHtml;
};
#endif
