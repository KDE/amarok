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

#include "AmarokManager.h"

#include "AmarokConfigWidget.h"
#include "AmarokProvider.h"

using namespace StatSyncing;

AMAROK_EXPORT_IMPORTER_PLUGIN( amarok, AmarokManager )

AmarokManager::AmarokManager( QObject *parent, const QVariantList &args )
    : ImporterManager( parent, args )
{
}

AmarokManager::~AmarokManager()
{
}

QString
AmarokManager::type() const
{
    return "AmarokImporter";
}

KPluginInfo
AmarokManager::pluginInfo() const
{
    return KPluginInfo( "amarok_importer-amarok.desktop", "services" );
}

QString
AmarokManager::prettyName() const
{
    return i18n( "Amarok 2.x" );
}

QString
AmarokManager::description() const
{
    return i18n( "Amarok 2.x Statistics Importer" );
}

KIcon
AmarokManager::icon() const
{
    return KIcon( "amarok" );
}

ProviderConfigWidget*
AmarokManager::configWidget( const QVariantMap &config )
{
    return new AmarokConfigWidget( config );
}

ImporterProviderPtr
AmarokManager::newInstance( const QVariantMap &config )
{
    return ProviderPtr( new AmarokProvider( config, this ) );
}
