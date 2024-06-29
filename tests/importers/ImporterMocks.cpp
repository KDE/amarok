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

#include "ImporterMocks.h"

#include "EngineController.h"
#include "core/support/Amarok.h"
#include "core/support/Components.h"

using namespace ::testing;

MockProvider::MockProvider( const QVariantMap &config, StatSyncing::ImporterManager *manager )
    : StatSyncing::ImporterProvider( config, manager )
{
}

QVariantMap
MockProvider::config() const
{
    return m_config;
}

StatSyncing::ImporterManager*
MockProvider::manager() const
{
    return m_manager;
}

MockManager::MockManager()
{
}

StatSyncing::ProviderPtrMap
MockManager::providers()
{
    return m_providers;
}

StatSyncing::ImporterProviderPtr
MockManager::concreteNewInstance( const QVariantMap &cfg )
{
    return StatSyncing::ImporterProviderPtr( new NiceMock<MockProvider>( cfg, this ) );
}

void
MockManager::providerForgottenProxy( const QString &providerId )
{
    return StatSyncing::ImporterManager::slotProviderForgotten( providerId );
}

MockController::MockController( QObject *parent )
    : StatSyncing::Controller( parent )
{
}

void
ImporterMocks::initTestCase()
{
    DefaultValue<QString>::Set( QString() );
    DefaultValue<QIcon>::Set( QIcon() );

    m_engineController = new EngineController;
    Amarok::Components::setEngineController( m_engineController );
}

void
ImporterMocks::init()
{
    m_mockManager = new NiceMock<MockManager>;
    ON_CALL( *m_mockManager, newInstance( _ ) )
            .WillByDefault( Invoke( m_mockManager, &MockManager::concreteNewInstance ) );

    ON_CALL( *m_mockManager, type() ).WillByDefault( Return( QStringLiteral("randomType") ) );

    QVariantMap cfg;
    cfg[QStringLiteral("uid")] = QStringLiteral( "providerUid" );
    m_mockProvider = new NiceMock<MockProvider>( cfg, m_mockManager );

    m_mockController = new NiceMock<MockController>;
    Amarok::Components::setStatSyncingController( m_mockController );
}

void
ImporterMocks::cleanup()
{
    delete m_mockProvider;
    m_mockProvider = nullptr;

    delete m_mockManager;
    m_mockManager = nullptr;

    Amarok::Components::setStatSyncingController( nullptr );
    delete m_mockController;
    m_mockController = nullptr;

    Amarok::config( QStringLiteral("Importers") ).deleteGroup();
}

void
ImporterMocks::cleanupTestCase()
{
    Amarok::config( QStringLiteral("StatSyncing") ).deleteGroup();
    Amarok::Components::setEngineController( nullptr );
    delete m_engineController;
}

