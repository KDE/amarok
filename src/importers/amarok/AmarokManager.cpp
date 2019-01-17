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


AmarokManager::AmarokManager()
    : ImporterManager()
{
}

QString
AmarokManager::type() const
{
    return QStringLiteral("AmarokImporter");
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

QIcon
AmarokManager::icon() const
{
    return QIcon::fromTheme( QStringLiteral("amarok") );
}

ProviderConfigWidget*
AmarokManager::configWidget( const QVariantMap &config )
{
    return new AmarokConfigWidget( config );
}

ImporterProviderPtr
AmarokManager::newInstance( const QVariantMap &config )
{
    return ImporterProviderPtr( new AmarokProvider( config, this ) );
}
