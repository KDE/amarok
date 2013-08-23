/****************************************************************************************
 * Copyright (c) 2013 Konrad Zemek <konrad.zemek@gmail.com>                             *
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

#include "ClementineManager.h"

#include "ClementineConfigWidget.h"
#include "ClementineProvider.h"

using namespace StatSyncing;

AMAROK_EXPORT_IMPORTER_PLUGIN( clementine, ClementineManager )

ClementineManager::ClementineManager( QObject *parent, const QVariantList &args )
    : ImporterManager( parent, args )
{
}

ClementineManager::~ClementineManager()
{
}

QString
ClementineManager::id() const
{
    return "ClementineImporter";
}

KPluginInfo
ClementineManager::pluginInfo() const
{
    return KPluginInfo( "amarok_importer-clementine.desktop", "services" );
}

QString
ClementineManager::prettyName() const
{
    return i18n( "Clementine" );
}

QString
ClementineManager::description() const
{
    return i18n( "Clementine Statistics Importer" );
}

KIcon
ClementineManager::icon() const
{
    return KIcon( "view-importers-clementine-amarok" );
}

ProviderConfigWidget*
ClementineManager::configWidget( const QVariantMap &config )
{
    return new ClementineConfigWidget( config );
}

ImporterProviderPtr
ClementineManager::newInstance( const QVariantMap &config )
{
    return ProviderPtr( new ClementineProvider( config, this ) );
}
