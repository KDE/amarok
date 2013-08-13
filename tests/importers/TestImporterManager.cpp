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

#include <qtest_kde.h>

QTEST_KDEMAIN_CORE( TestImporterManager )

using namespace ::testing;

void
TestImporterManager::constructorShouldSetPluginType()
{
    QCOMPARE( m_mockManager->pluginType(), Plugins::PluginFactory::Importer );
}

void
TestImporterManager::initShouldSetInitialized()
{
    m_mockManager->init();
    QVERIFY( m_mockManager->isInitialized() );
}

void
TestImporterManager::initShouldSetInfo()
{
    KPluginInfo expectedInfo( "testinfo", "services" );
    EXPECT_CALL( *m_mockManager, pluginInfo() ).WillOnce( Return( expectedInfo ) );

    m_mockManager->init();
    QCOMPARE( m_mockManager->info(), expectedInfo );
}

void
TestImporterManager::initShouldLoadSettings()
{
    QVariantMap config;
    config["uid"] = QString( "TestId" );
    config["custom"] = QString( "custom" );

    {
        NiceMock<MockManager> mock;

        EXPECT_CALL( mock, newInstance( Eq(config) ) ).WillOnce( Invoke( &mock, &MockManager::concreteNewInstance ) );
        EXPECT_CALL( mock, id() ).WillRepeatedly( Return( QString( "TestMockManager" ) ) );

        mock.init();
        mock.createProvider( config );
    }

    EXPECT_CALL( *m_mockManager, newInstance( Eq(config) ) );
    EXPECT_CALL( *m_mockManager, id() ).WillRepeatedly( Return( QString( "TestMockManager" ) ) );

    m_mockManager->init();

    Amarok::config( "Importers" ).deleteGroup();
    Amarok::config( "Importers.TestMockManager.TestId" ).deleteGroup();
}

void
TestImporterManager::creatingProviderShouldSetConfigAndParent()
{
    QVariantMap cfg;
    cfg["customField"] = QString( "I'm custom" );

    m_mockManager->init();
    StatSyncing::ProviderPtr providerPtr = m_mockManager->createProvider( cfg );

    QVERIFY( dynamic_cast<MockProvider*>(providerPtr.data())->config().contains( "customField" ) );
    QCOMPARE( dynamic_cast<MockProvider*>(providerPtr.data())->manager(), m_mockManager );
}

void
TestImporterManager::creatingProviderShouldSaveSettings()
{
    QVariantMap cfg;
    cfg["uid"] = QString( "TestId" );
    cfg["custom"] = QString( "custom" );

    EXPECT_CALL( *m_mockManager, id() ).WillRepeatedly( Return( QString( "TestMockManager" ) ) );

    m_mockManager->init();
    m_mockManager->createProvider( cfg );

    KConfigGroup config = Amarok::config( "Importers" );
    QVERIFY( config.exists() );
    QVERIFY( config.readEntry( "TestMockManager", QStringList() ).contains( "TestId" ) );

    config = Amarok::config( "Importers.TestMockManager.TestId" );
    QVERIFY( config.exists() );
    QCOMPARE( config.readEntry( "uid", QString() ), QString( "TestId" ) );
    QCOMPARE( config.readEntry( "custom", QString() ), QString( "custom" ) );

    Amarok::config( "Importers" ).deleteGroup();
    Amarok::config( "Importers.TestMockManager.TestId" ).deleteGroup();
}

void
TestImporterManager::creatingProviderShouldSaveGeneratedId()
{
    EXPECT_CALL( *m_mockManager, id() ).WillRepeatedly( Return( QString( "TestMockManager" ) ) );

    m_mockManager->init();
    m_mockManager->createProvider( QVariantMap() );

    KConfigGroup config = Amarok::config( "Importers.TestMockManager.TestId" );
    QVERIFY( config.exists() );
    QVERIFY( !config.readEntry( "uid", QString() ).isEmpty() );

    Amarok::config( "Importers" ).deleteGroup();
    Amarok::config( "Importers.TestMockManager.TestId" ).deleteGroup();
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
    StatSyncing::ProviderConfigWidget *ptr = 0;

    EXPECT_CALL( *m_mockManager, configWidget(_) )
            .WillOnce( Return( ptr ) );

    m_mockManager->init();
    QCOMPARE( m_mockManager->createConfigWidget(), ptr );
}

void
TestImporterManager::createProviderShouldNotCrashOnNull()
{
    EXPECT_CALL( *m_mockManager, newInstance(_) )
            .WillOnce( Return( StatSyncing::ProviderPtr( 0 ) ) );

    m_mockManager->init();
    QVERIFY( !m_mockManager->createProvider( QVariantMap() ) );
}

void
TestImporterManager::createProviderShouldReplaceProviderIfExists()
{
    EXPECT_CALL( *m_mockController, registerProvider(_) ).Times( 2 );

    m_mockManager->init();
    StatSyncing::ProviderPtr provider = m_mockManager->createProvider( QVariantMap() );

    QVERIFY( m_mockManager->providers().contains( provider->id() ) );
    QCOMPARE( m_mockManager->providers()[provider->id()], provider );

    EXPECT_CALL( *m_mockController, unregisterProvider( provider ) );

    QVariantMap cfg;
    cfg.insert( "uid", provider->id() );
    StatSyncing::ProviderPtr replaceProvider = m_mockManager->createProvider( cfg );

    QCOMPARE( m_mockManager->providers()[provider->id()], replaceProvider );
}

void
TestImporterManager::createProviderShouldRegisterProvider()
{
    QVariantMap cfg;
    cfg["uid"] = "uid";
    StatSyncing::ImporterProviderPtr providerPtr( new NiceMock<MockProvider>( cfg, m_mockManager ) );

    EXPECT_CALL( *m_mockManager, newInstance(_) ).WillOnce( Return( providerPtr ) );
    EXPECT_CALL( *m_mockController, registerProvider( Eq(providerPtr) ) );

    m_mockManager->init();
    m_mockManager->createProvider( QVariantMap() );
}

void
TestImporterManager::forgetProviderShouldUnregisterProvider()
{
    EXPECT_CALL( *m_mockController, registerProvider(_) );

    m_mockManager->init();
    StatSyncing::ProviderPtr providerPtr = m_mockManager->createProvider( QVariantMap() );

    EXPECT_CALL( *m_mockController, unregisterProvider( Eq(providerPtr) ) );
    m_mockManager->providerForgottenProxy( providerPtr->id() );
}

void
TestImporterManager::forgetProviderShouldForgetConfig()
{
    QVariantMap cfg;
    cfg.insert( "uid", "TestId" );

    EXPECT_CALL( *m_mockManager, id() ).WillRepeatedly( Return( QString( "TestMockManager" ) ) );

    m_mockManager->init();
    StatSyncing::ProviderPtr providerPtr = m_mockManager->createProvider( cfg );

    KConfigGroup config = Amarok::config( "Importers" );
    QVERIFY( config.exists() );
    QVERIFY( config.hasKey( "TestMockManager" ) );
    config = Amarok::config( "Importers.TestMockManager.TestId" );
    QVERIFY( config.exists() );
    QVERIFY( config.hasKey( "uid" ) );

    m_mockManager->providerForgottenProxy( providerPtr->id() );

    QVERIFY( !Amarok::config( "Importers" ).exists() );
    QVERIFY( !Amarok::config( "Importers.TestMockManager.TestId" ).exists() );
}

void
TestImporterManager::forgetProviderShouldHangleInvalidId()
{
    EXPECT_CALL( *m_mockController, unregisterProvider(_) ).Times( 0 );

    m_mockManager->init();
    m_mockManager->providerForgottenProxy( QString() );
}

void
TestImporterManager::managerShouldHandleMultipleProviders()
{
    EXPECT_CALL( *m_mockController, registerProvider(_) ).Times( 10 );

    QList<StatSyncing::ProviderPtr> providerPtrs;
    for( int i = 0; i < 10; ++i )
        providerPtrs << m_mockManager->createProvider( QVariantMap() );

    foreach( const StatSyncing::ProviderPtr &providerPtr, providerPtrs )
        QVERIFY( m_mockManager->providers().contains( providerPtr->id() ) );
}

#include "TestImporterManager.moc"
