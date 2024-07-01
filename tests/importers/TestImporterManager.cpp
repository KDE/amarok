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

#include "TestImporterManager.h"

#include "core/support/Amarok.h"
#include "core/support/Components.h"

#include <KLocalizedString>

#include <QStringList>
#include <QTest>


QTEST_GUILESS_MAIN( TestImporterManager )

using namespace ::testing;

void
TestImporterManager::initShouldLoadSettings()
{
    KLocalizedString::setApplicationDomain("amarok-test");
    QVariantMap config;
    config[QStringLiteral("uid")] = QStringLiteral( "TestId" );
    config[QStringLiteral("custom")] = QStringLiteral( "custom" );

    {
        NiceMock<MockManager> mock;

        EXPECT_CALL( mock, newInstance( Eq(config) ) ).WillOnce( Invoke( &mock, &MockManager::concreteNewInstance ) );
        EXPECT_CALL( mock, type() ).WillRepeatedly( Return( QStringLiteral( "TestMockManager" ) ) );

        mock.init();
        mock.createProvider( config );
    }

    EXPECT_CALL( *m_mockManager, newInstance( Eq(config) ) );
    EXPECT_CALL( *m_mockManager, type() ).WillRepeatedly( Return( QStringLiteral( "TestMockManager" ) ) );

    m_mockManager->init();
}

void
TestImporterManager::creatingProviderShouldSetConfigAndParent()
{
    QVariantMap cfg;
    cfg[QStringLiteral("customField")] = QStringLiteral( "I'm custom" );

    m_mockManager->init();
    StatSyncing::ProviderPtr providerPtr = m_mockManager->createProvider( cfg );

    QVERIFY( dynamic_cast<MockProvider*>(providerPtr.data())->config().contains( QStringLiteral( "customField" ) ) );
    QCOMPARE( dynamic_cast<MockProvider*>(providerPtr.data())->manager(), m_mockManager );
}

void
TestImporterManager::creatingProviderShouldSaveSettings()
{
    QVariantMap cfg;
    cfg[QStringLiteral("uid")] = QStringLiteral( "TestId" );
    cfg[QStringLiteral("custom")] = QStringLiteral( "custom" );

    EXPECT_CALL( *m_mockManager, type() ).WillRepeatedly( Return( QStringLiteral( "TestMockManager" ) ) );

    m_mockManager->init();
    m_mockManager->createProvider( cfg );

    KConfigGroup group = m_mockManager->providerConfig( QStringLiteral("TestId") );
    QVERIFY( group.exists() );
    QCOMPARE( group.readEntry( QStringLiteral("uid"), QString() ), QStringLiteral( "TestId" ) );
    QCOMPARE( group.readEntry( QStringLiteral("custom"), QString() ), QStringLiteral( "custom" ) );
}

void
TestImporterManager::creatingProviderShouldSaveGeneratedId()
{
    EXPECT_CALL( *m_mockManager, type() ).WillRepeatedly( Return( QStringLiteral( "TestMockManager" ) ) );

    QVERIFY( !m_mockManager->managerConfig().exists() );

    m_mockManager->init();
    StatSyncing::ProviderPtr provider = m_mockManager->createProvider( QVariantMap() );

    KConfigGroup group = m_mockManager->managerConfig();
    QVERIFY( group.exists() );
    QVERIFY( !group.groupList().empty() );

    group = m_mockManager->providerConfig( provider );
    QVERIFY( group.exists() );
    QCOMPARE( group.readEntry( "uid", QString() ), provider->id() );
}

void
TestImporterManager::creatingConfigWidgetShouldDelegate()
{
    StatSyncing::ProviderConfigWidget *ptr = reinterpret_cast<StatSyncing::ProviderConfigWidget*>( 0x19 );

    EXPECT_CALL( *m_mockManager, configWidget( QVariantMap() ) ).WillOnce( Return( ptr ) );

    m_mockManager->init();
    QCOMPARE( m_mockManager->createConfigWidget(), ptr );
}

void
TestImporterManager::createConfigWidgetShouldNotCrashOnNull()
{
    StatSyncing::ProviderConfigWidget *ptr = nullptr;

    EXPECT_CALL( *m_mockManager, configWidget(_) )
            .WillOnce( Return( ptr ) );

    m_mockManager->init();
    QCOMPARE( m_mockManager->createConfigWidget(), ptr );
}

void
TestImporterManager::createProviderShouldNotCrashOnNull()
{
    m_mockManager->init();

    EXPECT_CALL( *m_mockManager, newInstance(_) )
            .WillOnce( Return( QSharedPointer<StatSyncing::ImporterProvider>() ) );

    QVERIFY( !m_mockManager->createProvider( QVariantMap() ) );
}

void
TestImporterManager::createProviderShouldReplaceProviderIfExists()
{
    m_mockManager->init();

    EXPECT_CALL( *m_mockController, registerProvider(_) ).Times( 2 );

    StatSyncing::ProviderPtr provider = m_mockManager->createProvider( QVariantMap() );

    QVERIFY( m_mockManager->providers().contains( provider->id() ) );
    QCOMPARE( m_mockManager->providers()[provider->id()], provider );

    EXPECT_CALL( *m_mockController, unregisterProvider( provider ) );

    QVariantMap cfg;
    cfg.insert( QStringLiteral("uid"), provider->id() );
    StatSyncing::ProviderPtr replaceProvider = m_mockManager->createProvider( cfg );

    QCOMPARE( m_mockManager->providers()[provider->id()], replaceProvider );
}

void
TestImporterManager::createProviderShouldRegisterProvider()
{
    QVariantMap cfg;
    cfg[QStringLiteral("uid")] = QStringLiteral("uid");
    StatSyncing::ImporterProviderPtr providerPtr( new NiceMock<MockProvider>( cfg, m_mockManager ) );

    m_mockManager->init();

    EXPECT_CALL( *m_mockManager, newInstance(_) ).WillOnce( Return( providerPtr ) );
    EXPECT_CALL( *m_mockController, registerProvider( Eq(providerPtr) ) );

    m_mockManager->createProvider( QVariantMap() );
}

void
TestImporterManager::forgetProviderShouldUnregisterProvider()
{
    m_mockManager->init();

    EXPECT_CALL( *m_mockController, registerProvider(_) );

    StatSyncing::ProviderPtr providerPtr = m_mockManager->createProvider( QVariantMap() );

    EXPECT_CALL( *m_mockController, unregisterProvider( Eq(providerPtr) ) );
    m_mockManager->providerForgottenProxy( providerPtr->id() );
}

void
TestImporterManager::forgetProviderShouldForgetConfig()
{
    QVariantMap cfg;
    cfg.insert( QStringLiteral("uid"), QStringLiteral("TestId") );

    EXPECT_CALL( *m_mockManager, type() ).WillRepeatedly(
                                                 Return( QStringLiteral( "TestMockManager" ) ) );

    m_mockManager->init();
    StatSyncing::ProviderPtr providerPtr = m_mockManager->createProvider( cfg );

    KConfigGroup group = m_mockManager->providerConfig( providerPtr );
    QVERIFY( group.exists() );
    QVERIFY( group.hasKey( "uid" ) );

    m_mockManager->providerForgottenProxy( providerPtr->id() );

    QVERIFY( !m_mockManager->providerConfig( providerPtr ).exists() );
}

void
TestImporterManager::forgetProviderShouldHangleInvalidId()
{
    EXPECT_CALL( *m_mockController, unregisterProvider(_) ).Times( 0 );

    m_mockManager->init();
    m_mockManager->providerForgottenProxy( QString() );
}

void
TestImporterManager::forgetProviderShouldNotCauseOtherProvidersToBeForgotten()
{
    m_mockManager->init();

    StatSyncing::ProviderPtr providerPtr1 = m_mockManager->createProvider( QVariantMap() );
    StatSyncing::ProviderPtr providerPtr2 = m_mockManager->createProvider( QVariantMap() );

    QVERIFY( m_mockManager->providerConfig( providerPtr1 ).exists() );
    QVERIFY( m_mockManager->providerConfig( providerPtr2 ).exists() );

    m_mockManager->providerForgottenProxy( providerPtr1->id() );

    QVERIFY( !m_mockManager->providerConfig( providerPtr1 ).exists() );
    QVERIFY( m_mockManager->providerConfig( providerPtr2 ).exists() );
}

void
TestImporterManager::managerShouldHandleMultipleProviders()
{
    EXPECT_CALL( *m_mockController, registerProvider(_) ).Times( 10 );

    QList<StatSyncing::ProviderPtr> providerPtrs;
    for( int i = 0; i < 10; ++i )
        providerPtrs << m_mockManager->createProvider( QVariantMap() );

    for( auto const &providerPtr : providerPtrs )
        QVERIFY( m_mockManager->providers().contains( providerPtr->id() ) );
}

