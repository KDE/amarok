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

#include "ITunesManager.h"

#include "ITunesConfigWidget.h"
#include "ITunesProvider.h"

using namespace StatSyncing;

AMAROK_EXPORT_IMPORTER_PLUGIN( itunes, ITunesManager )

ITunesManager::ITunesManager( QObject *parent, const QVariantList &args )
    : ImporterManager( parent, args )
{
}

ITunesManager::~ITunesManager()
{
}

QString
ITunesManager::id() const
{
    return "ITunesImporter";
}

KPluginInfo
ITunesManager::pluginInfo() const
{
    return KPluginInfo( "amarok_importer-itunes.desktop", "services" );
}

QString
ITunesManager::prettyName() const
{
    return i18n( "Apple iTunes" );
}

QString
ITunesManager::description() const
{
    return i18n( "iTunes Statistics Importer" );
}

KIcon
ITunesManager::icon() const
{
    return KIcon( "media-album-track" ); //TODO
}

ProviderConfigWidget*
ITunesManager::configWidget( const QVariantMap &config )
{
    return new ITunesConfigWidget( config );
}

ImporterProviderPtr
ITunesManager::newInstance( const QVariantMap &config )
{
    return ImporterProviderPtr( new ITunesProvider( config, this ) );
}
