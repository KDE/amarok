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

#include "RhythmboxManager.h"

#include "RhythmboxConfigWidget.h"
#include "RhythmboxProvider.h"

using namespace StatSyncing;

AMAROK_EXPORT_IMPORTER_PLUGIN( rhythmbox, RhythmboxManager )

RhythmboxManager::RhythmboxManager( QObject *parent, const QVariantList &args )
    : ImporterManager( parent, args )
{
}

RhythmboxManager::~RhythmboxManager()
{
}

QString
RhythmboxManager::id() const
{
    return "RhythmboxImporter";
}

KPluginInfo
RhythmboxManager::pluginInfo() const
{
    return KPluginInfo( "amarok_importer-rhythmbox.desktop", "services" );
}

QString
RhythmboxManager::prettyName() const
{
    return i18n( "Rhythmbox" );
}

QString
RhythmboxManager::description() const
{
    return i18n( "Rhythmbox Statistics Importer" );
}

KIcon
RhythmboxManager::icon() const
{
    return KIcon( "view-importers-rhythmbox-amarok" );
}

ProviderConfigWidget*
RhythmboxManager::configWidget( const QVariantMap &config )
{
    return new RhythmboxConfigWidget( config );
}

ImporterProviderPtr
RhythmboxManager::newInstance( const QVariantMap &config )
{
    return ImporterProviderPtr( new RhythmboxProvider( config, this ) );
}
