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

#ifndef STATSYNCING_IMPORTER_PROVIDER_H
#define STATSYNCING_IMPORTER_PROVIDER_H

#include "statsyncing/Provider.h"

#include "amarok_export.h"

namespace StatSyncing
{

class ImporterManager;

/**
 * The ImporterProvider class is a base class for every @see StatSyncing::Provider
 * derived statistic importer. It serves to reduce boilerplate by offering a common
 * implementation of some StatSyncing::Provider methods.
 *
 * For details about methods' purpose, and other methods that need to be implemented
 * in a concrete ImporterProvider, see StatSyncing::Provider documentation.
 */
class AMAROK_EXPORT ImporterProvider : public Provider
{
    Q_OBJECT

    /// Give ImporterManager access to config field in order to save and restore state.
    friend class ImporterManager;

public:
    /**
     * The constructor stores @param config as a protected @see m_config variable, and
     * @param manager as @see m_manager. If config["uid"] isn't set, it's generated here.
     */
    ImporterProvider( const QVariantMap &config, ImporterManager *manager );
    virtual ~ImporterProvider();

    /**
     * Provider's unique id which may be used as a key for configuration storage.
     * Returns m_config["uid"] by default.
     */
    virtual QString id() const;

    /**
     * Description of the provider. Returns m_importer->description() by default.
     */
    virtual QString description() const;

    /**
     * Provider's icon. Returns m_importer->icon() by default.
     */
    virtual QIcon icon() const;

    /**
     * Provider's name as displayed in Amarok's Metadata Configuration tab. Returns
     * m_config["name"] by default.
     */
    virtual QString prettyName() const;

    /**
     * Returns true if provider is configurable. Returns true by default.
     */
    virtual bool isConfigurable() const;

    /**
     * Returns configuration widget used to reconfigure this provider. By default
     * delegates to m_importer->getConfigWidget( m_config ).
     */
    virtual ProviderConfigWidget *configWidget();

    /**
     * Reconfigures current provider. An ImporterManager subclass handles the
     * task, _recreating_ this provider with new configuration. Please note that
     * m_config["uid"] is not subject to reconfiguration.
     */
    virtual void reconfigure( const QVariantMap &config );

    /**
     * Determines if this provider should participate in statistics synchronization
     * by default. By default returns StatSyncing::Provider::NoByDefault .
     */
    virtual Preference defaultPreference();

Q_SIGNALS:
    void reconfigurationRequested( const QVariantMap &config );

protected:
    /**
     * Configuration of this provider. It is saved and restored by an ImporterManager
     * subclass.
     */
    QVariantMap m_config;
    ImporterManager *m_manager;
};

} // namespace StatSyncing

#endif // STATSYNCING_IMPORTER_PROVIDER_H
