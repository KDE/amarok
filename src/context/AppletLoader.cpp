/****************************************************************************************
 * Copyright (c) 2017 Malte Veerman <malte.veerman@gmail.com>                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "AppletLoader"

#include "AppletLoader.h"

#include "AmarokContextPackageStructure.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include <KPackage/PackageLoader>

using namespace Context;

AppletLoader::AppletLoader(QObject *parent) : QObject(parent)
{
    findApplets();
}

AppletLoader::~AppletLoader()
{
}

QList<KPluginMetaData> AppletLoader::applets() const
{
    return m_applets;
}

QList<KPluginMetaData> Context::AppletLoader::enabledApplets() const
{
    QList<KPluginMetaData> list;
    QStringList enabledApplets = Amarok::config(QStringLiteral("Context")).readEntry("enabledApplets", QStringList());

    for (const auto &applet : m_applets)
    {
        if (enabledApplets.contains(applet.pluginId()))
            list << applet;
    }
    return list;
}

void AppletLoader::findApplets()
{
    DEBUG_BLOCK

    auto loader = KPackage::PackageLoader::self();
    auto structure = new AmarokContextPackageStructure;
    loader->addKnownPackageStructure( QStringLiteral("Amarok/ContextApplet"), structure );
    m_applets = loader->findPackages( QStringLiteral("Amarok/ContextApplet"), QString() );
    Q_EMIT finished(m_applets);

    for (const auto &applet : m_applets)
    {
        debug() << "Applet found:" << applet.name();
    }
    debug() << "Number of applets found:" << m_applets.size();

    if( m_applets.isEmpty() )
        warning() << "No applets found";
}
