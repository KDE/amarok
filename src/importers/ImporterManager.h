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

#include "statsyncing/Provider.h"

#include "amarok_export.h"

/**
  * This macro needs to be expanded exactly once for a single importer. The best practice
  * is to put it at the beginning of the .cpp file of concrete ImporterManager
  * implementation.
  */
#define AMAROK_EXPORT_IMPORTER_PLUGIN( libname, FactoryClass ) \
    K_PLUGIN_FACTORY( factory, registerPlugin<FactoryClass>(); ) \
    K_EXPORT_PLUGIN( factory( "amarok_importer-" #libname ) )

namespace StatSyncing
{

class ImporterProvider;
typedef QExplicitlySharedDataPointer<ImporterProvider> ImporterProviderPtr;
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
    ImporterManager( QObject *parent, const QVariantList &args );

    /**
     * Destructor.
     */
    virtual ~ImporterManager();

    /**
     * Loads up saved configuration, for every retrieved config calls
     * createProvider( config ), and then registers created providers with
     * StatSyncing::Controller. This method is called by PluginManager.
     */
    virtual void init();

    /**
     * Basic implementation for StatSyncing::ProviderFactory createConfigWidget() method,
     * used for configuring new providers. By default calls @see getConfigWidget with empty
     * config.
     */
    virtual ProviderConfigWidget *createConfigWidget();

    /**
     * Returns a configuration widget prepopulated with given config values.
     */
    virtual ProviderConfigWidget *configWidget( const QVariantMap &config
                                                                   = QVariantMap() ) = 0;
public slots:
    /**
     * Creates a new provider by calling newInstance and saves the config to the disk.
     * The created provider is registered with the StatSyncing::Controller
     *
     * This method can also be used to replace existing provider instances.
     */
    virtual ProviderPtr createProvider( QVariantMap config );

protected:
    /**
     * Return the KPluginInfo for this importer. The KPluginInfo should contain the
     * name of this importer's .desktop file and plugin's type (typically "services").
     * This function's return value will initialize m_info variable of PluginFactory.
     */
    virtual KPluginInfo pluginInfo() const = 0;

    /**
     * Return a new provider instance.
     */
    virtual ImporterProviderPtr newInstance( const QVariantMap &config ) = 0;

    /**
     * A list of every provider associated with this ImporterManager instance.
     * After init() it's populated by loaded providers.
     */
    ProviderPtrMap m_providers;

protected slots:
    /**
     * ProviderImporter listens to StatSyncing::Config's providerForgotten signal, and
     * unregisters and removes managed providers if they're forgotten.
     */
    virtual void slotProviderForgotten( const QString &providerId );
};

} // namespace StatSyncing

#endif // STATSYNCING_IMPORTER_MANAGER_H
