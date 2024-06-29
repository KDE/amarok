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

#include "TestImporterProvider.h"

#include "core/support/Amarok.h"
#include "core/support/Components.h"

#include <QSignalSpy>
#include <QTest>


QTEST_GUILESS_MAIN( TestImporterProvider )

using namespace ::testing;

void
TestImporterProvider::constructorShouldSetConfigAndManager()
{
    QVariantMap cfg;
    cfg[QStringLiteral("nanananana")] = QStringLiteral( "Batman" );
    MockProvider provider( cfg, m_mockManager );

    QVERIFY( provider.config().contains( QStringLiteral( "nanananana" ) ) );
    QCOMPARE( provider.manager(), m_mockManager );
}

void
TestImporterProvider::constructorShouldSetUidIfNotSet()
{
    QVERIFY( !MockProvider( QVariantMap(), nullptr ).id().isEmpty() );
}

void
TestImporterProvider::idShouldReturnConfiguredId()
{
    QVariantMap cfg;
    cfg[QStringLiteral("uid")] = QStringLiteral( "Joker" );

    QCOMPARE( MockProvider( cfg, nullptr ).config(), cfg );
}

void
TestImporterProvider::descriptionShouldDelegateToManager()
{
    EXPECT_CALL( *m_mockManager, description() ).WillOnce( Return( QStringLiteral( "Ivy" ) ) );
    QCOMPARE( m_mockProvider->description(), QStringLiteral( "Ivy" ) );
}

void
TestImporterProvider::iconShouldDelegateToManager()
{
    EXPECT_CALL( *m_mockManager, icon() ).WillOnce( Return( QIcon::fromTheme( QStringLiteral("amarok") ) ) );
    QCOMPARE( m_mockProvider->icon().name(), QIcon::fromTheme( QStringLiteral("amarok") ).name() );
}

void
TestImporterProvider::nameShouldReturnConfiguredName()
{
    QVariantMap cfg;
    cfg[QStringLiteral("uid")] = QStringLiteral( "Bane" );
    cfg[QStringLiteral("name")] = QStringLiteral( "Ra's" );
    MockProvider provider( cfg, m_mockManager );

    QCOMPARE( provider.prettyName(), QStringLiteral( "Ra's" ) );
}

void
TestImporterProvider::nameShouldNotCrashIfNameIsNotConfigured()
{
    QVariantMap cfg;
    cfg[QStringLiteral("uid")] = QStringLiteral( "TwoFace" );
    MockProvider provider( cfg, m_mockManager );

    QCOMPARE( provider.prettyName(), QString() );
}

void
TestImporterProvider::isConfigurableShouldReturnTrue()
{
    QVERIFY( m_mockProvider->isConfigurable() );
}

void
TestImporterProvider::configWidgetShouldDelegateToManager()
{
    StatSyncing::ProviderConfigWidget *widget = nullptr;
    EXPECT_CALL( *m_mockManager, configWidget( Eq(m_mockProvider->config()) ) )
            .WillOnce( Return( widget ) );
    QCOMPARE( m_mockProvider->configWidget(), widget );
}

void
TestImporterProvider::reconfigureShouldEmitSignal()
{
    QVariantMap cfg = m_mockProvider->config();
    cfg[QStringLiteral("customField")] = QStringLiteral( "Selena" );

    QSignalSpy spy( m_mockProvider, &MockProvider::reconfigurationRequested );
    m_mockProvider->reconfigure( cfg );

    QCOMPARE( spy.count(), 1 );
    QCOMPARE( spy.takeFirst().at( 0 ).toMap(), cfg );
}

void
TestImporterProvider::reconfigureShouldNotEmitSignalOnDifferentUid()
{
    QVariantMap cfg;
    cfg[QStringLiteral("uid")] = QStringLiteral("Different");

    QSignalSpy spy( m_mockProvider, &MockProvider::reconfigurationRequested );
    m_mockProvider->reconfigure( cfg );

    QCOMPARE( spy.count(), 0 );
}

void
TestImporterProvider::defaultPreferenceShouldReturnNoByDefault()
{
    QCOMPARE( m_mockProvider->defaultPreference(), StatSyncing::Provider::NoByDefault );
}

