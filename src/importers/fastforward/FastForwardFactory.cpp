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

#include "FastForwardFactory.h"

#include "FastForwardProvider.h"
#include "FastForwardConfigWidget.h"

#include <QDebug>

namespace StatSyncing
{

AMAROK_EXPORT_IMPORTER_PLUGIN( fastforward, FastForwardFactory )

FastForwardFactory::FastForwardFactory( QObject *parent, const QVariantList &args )
    : ImporterFactory( parent, args )
{

}

FastForwardFactory::~FastForwardFactory()
{

}

QString
FastForwardFactory::id() const
{
    return "FastForwardFactory";
}

KPluginInfo FastForwardFactory::info() const
{
    return KPluginInfo( "amarok_importer-fastforward.desktop", "services" );
}

QString
FastForwardFactory::prettyName() const
{
    return i18n( "Amarok 1.4 (FastForward)" );
}

QString FastForwardFactory::description() const
{
    return i18n( "Amarok FastForward description" );
}

KIcon
FastForwardFactory::icon() const
{
    return KIcon( "amarok" );
}

ProviderConfigWidget*
FastForwardFactory::getConfigWidget( const QVariantMap &config )
{
    return new FastForwardConfigWidget( config );
}

ProviderPtr
FastForwardFactory::createProvider( const QVariantMap &config )
{
    return ProviderPtr( new FastForwardProvider( config, this ) );
}

} // namespace StatSyncing
