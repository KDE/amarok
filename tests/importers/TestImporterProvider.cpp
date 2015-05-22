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

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( TestImporterProvider )

using namespace ::testing;

void
TestImporterProvider::constructorShouldSetConfigAndManager()
{
    QVariantMap cfg;
    cfg["nanananana"] = QString( "Batman" );
    MockProvider provider( cfg, m_mockManager );

    QVERIFY( provider.config().contains( QString( "nanananana" ) ) );
    QCOMPARE( provider.manager(), m_mockManager );
}

void
TestImporterProvider::constructorShouldSetUidIfNotSet()
{
    QVERIFY( !MockProvider( QVariantMap(), 0 ).id().isEmpty() );
}

void
TestImporterProvider::idShouldReturnConfiguredId()
{
    QVariantMap cfg;
    cfg["uid"] = QString( "Joker" );

    QCOMPARE( MockProvider( cfg, 0 ).config(), cfg );
}

void
TestImporterProvider::descriptionShouldDelegateToManager()
{
    EXPECT_CALL( *m_mockManager, description() ).WillOnce( Return( QString( "Ivy" ) ) );
    QCOMPARE( m_mockProvider->description(), QString( "Ivy" ) );
}

void
TestImporterProvider::iconShouldDelegateToManager()
{
    EXPECT_CALL( *m_mockManager, icon() ).WillOnce( Return( KIcon( "amarok" ) ) );
    QCOMPARE( m_mockProvider->icon().name(), KIcon( "amarok" ).name() );
}

void
TestImporterProvider::nameShouldReturnConfiguredName()
{
    QVariantMap cfg;
    cfg["uid"] = QString( "Bane" );
    cfg["name"] = QString( "Ra's" );
    MockProvider provider( cfg, m_mockManager );

    QCOMPARE( provider.prettyName(), QString( "Ra's" ) );
}

void
TestImporterProvider::nameShouldNotCrashIfNameIsNotConfigured()
{
    QVariantMap cfg;
    cfg["uid"] = QString( "TwoFace" );
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
    StatSyncing::ProviderConfigWidget *widget = 0;
    EXPECT_CALL( *m_mockManager, configWidget( Eq(m_mockProvider->config()) ) )
            .WillOnce( Return( widget ) );
    QCOMPARE( m_mockProvider->configWidget(), widget );
}

void
TestImporterProvider::reconfigureShouldEmitSignal()
{
    QVariantMap cfg = m_mockProvider->config();
    cfg["customField"] = QString( "Selena" );

    QSignalSpy spy( m_mockProvider, SIGNAL(reconfigurationRequested(QVariantMap)) );
    m_mockProvider->reconfigure( cfg );

    QCOMPARE( spy.count(), 1 );
    QCOMPARE( spy.takeFirst().at( 0 ).toMap(), cfg );
}

void
TestImporterProvider::reconfigureShouldNotEmitSignalOnDifferentUid()
{
    QVariantMap cfg;
    cfg["uid"] = "Different";

    QSignalSpy spy( m_mockProvider, SIGNAL(reconfigurationRequested(QVariantMap)) );
    m_mockProvider->reconfigure( cfg );

    QCOMPARE( spy.count(), 0 );
}

void
TestImporterProvider::defaultPreferenceShouldReturnNoByDefault()
{
    QCOMPARE( m_mockProvider->defaultPreference(), StatSyncing::Provider::NoByDefault );
}

