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

#include "BansheeManager.h"

#include "BansheeConfigWidget.h"
#include "BansheeProvider.h"

using namespace StatSyncing;

AMAROK_EXPORT_IMPORTER_PLUGIN( banshee, BansheeManager )

BansheeManager::BansheeManager( QObject *parent, const QVariantList &args )
    : ImporterManager( parent, args )
{
}

BansheeManager::~BansheeManager()
{
}

QString
BansheeManager::id() const
{
    return "BansheeImporter";
}

KPluginInfo
BansheeManager::pluginInfo() const
{
    return KPluginInfo( "amarok_importer-banshee.desktop", "services" );
}

QString
BansheeManager::prettyName() const
{
    return i18n( "Banshee" );
}

QString
BansheeManager::description() const
{
    return i18n( "Banshee Statistics Importer" );
}

KIcon
BansheeManager::icon() const
{
    return KIcon( "view-importers-banshee-amarok" );
}

ProviderConfigWidget*
BansheeManager::configWidget( const QVariantMap &config )
{
    return new BansheeConfigWidget( config );
}

ImporterProviderPtr
BansheeManager::newInstance( const QVariantMap &config )
{
    return ProviderPtr( new BansheeProvider( config, this ) );
}
