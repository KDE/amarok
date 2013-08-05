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

#include "FastForwardManager.h"

#include "FastForwardConfigWidget.h"
#include "FastForwardProvider.h"

using namespace StatSyncing;

AMAROK_EXPORT_IMPORTER_PLUGIN( fastforward, FastForwardManager )

FastForwardManager::FastForwardManager( QObject *parent, const QVariantList &args )
    : ImporterManager( parent, args )
{
}

FastForwardManager::~FastForwardManager()
{
}

QString
FastForwardManager::id() const
{
    return "FastForwardImporter";
}

KPluginInfo
FastForwardManager::pluginInfo() const
{
    return KPluginInfo( "amarok_importer-fastforward.desktop", "services" );
}

QString
FastForwardManager::prettyName() const
{
    return i18n( "Amarok 1.4 (FastForward)" );
}

QString
FastForwardManager::description() const
{
    return i18n( "Amarok 1.4 Statistics Importer" );
}

KIcon
FastForwardManager::icon() const
{
    return KIcon( "amarok" );
}

ProviderConfigWidget*
FastForwardManager::configWidget( const QVariantMap &config )
{
    return new FastForwardConfigWidget( config );
}

ImporterProviderPtr
FastForwardManager::newInstance( const QVariantMap &config )
{
    return ProviderPtr( new FastForwardProvider( config, this ) );
}
