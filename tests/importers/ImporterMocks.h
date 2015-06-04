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

#ifndef IMPORTER_MOCKS_H
#define IMPORTER_MOCKS_H

#include "importers/ImporterManager.h"
#include "importers/ImporterProvider.h"
#include "statsyncing/Controller.h"

#include <gmock/gmock.h>

class EngineController;
namespace StatSyncing
{
    class Controller;
}

class MockProvider : public StatSyncing::ImporterProvider
{
public:
    MockProvider( const QVariantMap &config, StatSyncing::ImporterManager *manager );
    QVariantMap config() const;
    StatSyncing::ImporterManager *manager() const;

    MOCK_CONST_METHOD0( reliableTrackMetaData, qint64() );
    MOCK_CONST_METHOD0( writableTrackStatsData, qint64() );
    MOCK_METHOD0( artists, QSet<QString>() );
    MOCK_METHOD1( artistTracks, StatSyncing::TrackList(const QString&) );
};

class MockManager : public StatSyncing::ImporterManager
{
public:
    MockManager();

    StatSyncing::ProviderPtrMap providers();
    StatSyncing::ImporterProviderPtr concreteNewInstance( const QVariantMap &cfg );
    void providerForgottenProxy( const QString &providerId );

    using StatSyncing::ImporterManager::managerConfig;
    using StatSyncing::ImporterManager::providerConfig;

    MOCK_CONST_METHOD0( type, QString() );
    MOCK_CONST_METHOD0( description, QString() );
    MOCK_CONST_METHOD0( prettyName, QString() );
    MOCK_CONST_METHOD0( icon, QIcon() );
    MOCK_METHOD1( configWidget, StatSyncing::ProviderConfigWidget*(const QVariantMap&) );
    MOCK_CONST_METHOD0( pluginInfo, KPluginInfo() );
    MOCK_METHOD1( newInstance, StatSyncing::ImporterProviderPtr(const QVariantMap&) );
};

class MockController : public StatSyncing::Controller
{
public:
    MockController( QObject *parent = 0 );

    MOCK_METHOD1( registerProvider, void(const StatSyncing::ProviderPtr&) );
    MOCK_METHOD1( unregisterProvider, void(const StatSyncing::ProviderPtr&) );
};

class ImporterMocks : public QObject
{
    Q_OBJECT

protected:
    EngineController *m_engineController;
    MockController *m_mockController;
    MockManager *m_mockManager;
    MockProvider *m_mockProvider;

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();
    void cleanupTestCase();
};

#endif // IMPORTER_MOCKS_H
