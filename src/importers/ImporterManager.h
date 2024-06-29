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

#ifndef STATSYNCING_IMPORTER_MANAGER_H
#define STATSYNCING_IMPORTER_MANAGER_H

#include "statsyncing/ProviderFactory.h"

#include "amarok_export.h"

#include <KConfigGroup>


namespace StatSyncing
{

class ImporterProvider;
typedef QSharedPointer<ImporterProvider> ImporterProviderPtr;
typedef QMap<QString, ProviderPtr> ProviderPtrMap;

/**
 * The ImporterManager class is a base class for every @see StatSyncing::ProviderFactory
 * derived importer provider factory, which in turn is derived from
 * Plugins::PluginFactory .
 *
 * For details about methods' purpose, and other methods that need to be implemented
 * in a concrete ImporterManager see StatSyncing::ProviderFactory documentation.
 */
class AMAROK_EXPORT ImporterManager : public ProviderFactory
{
    Q_OBJECT

public:
    /**
     * Constructor. Sets the Plugins::PluginFactory m_type variable to type Importer
     */
    ImporterManager();

    /**
     * Destructor.
     */
    ~ImporterManager() override;

    /**
     * Loads up saved configuration, for every retrieved config calls
     * createProvider( config ), and then registers created providers with
     * StatSyncing::Controller. This method is called by PluginManager.
     */
    void init() override;

    /**
     * Basic implementation for StatSyncing::ProviderFactory createConfigWidget() method,
     * used for configuring new providers. By default calls @see getConfigWidget with
     * empty config parameter.
     */
    ProviderConfigWidget *createConfigWidget() override;

    /**
     * Returns a configuration widget prepopulated with given config values.
     */
    virtual ProviderConfigWidget *configWidget( const QVariantMap &config
                                                                    = QVariantMap() ) = 0;
public Q_SLOTS:
    /**
     * Creates a new provider by calling newInstance and saves the config to the disk.
     * The created provider is registered with the StatSyncing::Controller
     *
     * This method can also be used to replace existing provider instances.
     */
    ProviderPtr createProvider( const QVariantMap &config ) override;

protected:
    /**
     * Convenience method returning a config group for this manager.
     */
    KConfigGroup managerConfig() const;

    /**
     * Convenience method returning a config group for a given @param providerId .
     */
    KConfigGroup providerConfig( const QString &providerId ) const;

    /**
     * Overload of @see ImporterManager::providerConfig( const QString ) .
     */
    KConfigGroup providerConfig( const ProviderPtr &provider ) const;

    /**
     * Return a new provider instance.
     */
    virtual ImporterProviderPtr newInstance( const QVariantMap &config ) = 0;

    /**
     * A list of every provider associated with this ImporterManager instance.
     * After init() it's populated by loaded providers.
     */
    ProviderPtrMap m_providers;

protected Q_SLOTS:
    /**
     * ProviderImporter listens to StatSyncing::Config's providerForgotten signal, and
     * unregisters and removes managed providers if they're forgotten.
     */
    virtual void slotProviderForgotten( const QString &providerId );
};

} // namespace StatSyncing

#endif // STATSYNCING_IMPORTER_MANAGER_H
