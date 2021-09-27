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

#include "ImporterManager.h"
#include "core/support/Debug.h"

#include <QRandomGenerator>

using namespace StatSyncing;

ImporterProvider::ImporterProvider( const QVariantMap &config, ImporterManager *manager )
    : m_config( config )
    , m_manager( manager )
{
    if( !m_config.contains( QStringLiteral("uid") ) )
        m_config.insert( QStringLiteral("uid"), QRandomGenerator::global()->generate() );

    if( m_manager == nullptr )
        warning() << __PRETTY_FUNCTION__ << "manager pointer is not set";
}

ImporterProvider::~ImporterProvider()
{
}

QString
StatSyncing::ImporterProvider::id() const
{
    return m_config.value( QStringLiteral("uid") ).toString();
}

QString
ImporterProvider::description() const
{
    return m_manager ? m_manager->description() : QString();
}

QIcon
ImporterProvider::icon() const
{
    return m_manager ? m_manager->icon() : QIcon();
}

QString
ImporterProvider::prettyName() const
{
    return m_config.value( QStringLiteral("name") ).toString();
}

bool
ImporterProvider::isConfigurable() const
{
    return true;
}

ProviderConfigWidget*
ImporterProvider::configWidget()
{
    Q_ASSERT( m_manager );
    return m_manager ? m_manager->configWidget( m_config ) : nullptr;
}

void
ImporterProvider::reconfigure( const QVariantMap &config )
{
    if( config.value( QStringLiteral("uid") ) == m_config.value( QStringLiteral("uid") ) )
        Q_EMIT reconfigurationRequested( config );
    else
        warning() << __PRETTY_FUNCTION__ << "reconfigure called with different provider"
                  << "uid!";
}

Provider::Preference
ImporterProvider::defaultPreference()
{
    return NoByDefault;
}
