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

    MOCK_METHOD( qint64, reliableTrackMetaData, (), (const, override) );
    MOCK_METHOD( qint64, writableTrackStatsData, (), (const, override) );
    MOCK_METHOD(  QSet<QString>, artists, (), (override) );
    MOCK_METHOD( StatSyncing::TrackList, artistTracks, (const QString&), (override) );
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

    MOCK_METHOD( QString, type, (), (const, override) );
    MOCK_METHOD( QString, description, (), (const, override) );
    MOCK_METHOD( QString, prettyName, (), (const, override) );
    MOCK_METHOD( QIcon, icon, (), (const, override) );
    MOCK_METHOD( StatSyncing::ProviderConfigWidget*, configWidget, (const QVariantMap&), (override) );
    MOCK_METHOD( StatSyncing::ImporterProviderPtr, newInstance, (const QVariantMap&), (override) );
};

class MockController : public StatSyncing::Controller
{
public:
    MockController( QObject *parent = nullptr );

    MOCK_METHOD( void, registerProvider, (const StatSyncing::ProviderPtr&), (override) );
    MOCK_METHOD( void, unregisterProvider, (const StatSyncing::ProviderPtr&), (override) );
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
