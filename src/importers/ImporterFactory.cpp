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

#include "ImporterFactory.h"

#include "core/support/Amarok.h"
#include "core/support/Components.h"
#include "statsyncing/Controller.h"

#include <QVariant>
#include <QStringList>

#include <KConfigGroup>

namespace StatSyncing
{

ImporterFactory::ImporterFactory( QObject *parent, const QVariantList &args )
    : ProviderFactory( parent, args )
{
    m_type = Importer;
}

ImporterFactory::~ImporterFactory()
{
}

void
ImporterFactory::init()
{
    m_info = info();

    KConfigGroup group = Amarok::config( "Importers" );
    QStringList providersConfigs =
            group.readEntry( id(), QStringList() );

    Controller *controller = Amarok::Components::statSyncingController();
    foreach( QVariant providerConfigVariant, providersConfigs )
    {
        const QVariantMap providerConfig = providerConfigVariant.toMap();
        ProviderPtr provider = createProvider( providerConfig );
        m_providers << provider;

        if( controller )
            controller->registerProvider( provider );
    }

    m_initialized = true;
}

ProviderConfigWidget*
ImporterFactory::createConfigWidget()
{
    return getConfigWidget( QVariantMap() );
}

} // namespace StatSyncing
