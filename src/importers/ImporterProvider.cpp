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

#include "ImporterProvider.h"

#include "ImporterFactory.h"

namespace StatSyncing
{

ImporterProvider::ImporterProvider( const QVariantMap &config,
                                    ImporterFactory *importer )
    : m_config( config )
    , m_importer( importer )
{
    if( !m_config.contains( "uid" ) )
        m_config.insert( "uid", qrand() );
}

ImporterProvider::~ImporterProvider()
{
}

QString
ImporterProvider::description() const
{
    return m_importer->description();
}

KIcon
ImporterProvider::icon() const
{
    return m_importer->icon();
}

QString
ImporterProvider::prettyName() const
{
    return m_config["name"].toString();
}

bool
ImporterProvider::isConfigurable() const
{
    return true;
}

ProviderConfigWidget*
ImporterProvider::configWidget()
{
    return m_importer->getConfigWidget( m_config );
}

Provider::Preference
ImporterProvider::defaultPreference()
{
    return Ask;
}

} // namespace StatSyncing
