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

#ifndef STATSYNCING_IMPORTER_FACTORY_H
#define STATSYNCING_IMPORTER_FACTORY_H

#include "statsyncing/Provider.h"

#include "amarok_export.h"

/**
  * This macro needs to be called once for every importer, in a single translation unit.
  */
#define AMAROK_EXPORT_IMPORTER_PLUGIN( libname, FactoryClass )\
    K_PLUGIN_FACTORY( factory, registerPlugin<FactoryClass>(); )\
    K_EXPORT_PLUGIN( factory( "amarok_importer-" #libname ) )

namespace StatSyncing
{

/**
 * The ImporterFactory class is a base class for every @see StatSyncing::ProviderFactory
 * derived importer provider factory, which in turn is derived from
 * Plugins::PluginFactory .
 *
 * For details about methods' purpose, and other methods that need to be implemented
 * in a concrete ImporterFactory see StatSyncing::ProviderFactory documentation.
 */
class AMAROK_EXPORT ImporterFactory : public ProviderFactory
{
public:
    /**
     * Constructor. Sets the Plugins::PluginFactory m_type variable to type Importer
     */
    ImporterFactory( QObject *parent, const QVariantList &args );
    virtual ~ImporterFactory();

    /**
     * Loads up saved configuration, for every retrieved config calls
     * createProvider( config ), and then registers created providers with
     * StatSyncing::Controller. This method is called by PluginManager.
     */
    virtual void init();

    /**
     * Return the KPluginInfo for this importer. The KPluginInfo should contain the
     * name of this importer's .desktop file and plugin's type (typically a "service").
     * This function's return value will initialize m_info variable of PluginFactory.
     */
    virtual KPluginInfo info() const = 0;

    /**
     * Basic implementation for StatSyncing::ProviderFactory createConfigWidget() method,
     * used for configuring new providers. By default calls @see getConfigWidget with empty
     * config.
     */
    virtual ProviderConfigWidget *createConfigWidget();

    /**
     * Returns a configuration widget prepopulated with given config values.
     */
    virtual ProviderConfigWidget *getConfigWidget( const QVariantMap &config
                                                                   = QVariantMap() ) = 0;

protected:
    /**
     * A list of every provider associated with this ImporterFactory instance.
     * After init() it's populated by loaded providers.
     */
    ProviderPtrList m_providers;
};

} // namespace StatSyncing

#endif // STATSYNCING_IMPORTER_FACTORY_H
