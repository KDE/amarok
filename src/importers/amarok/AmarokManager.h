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

#ifndef STATSYNCING_AMAROK_MANAGER_H
#define STATSYNCING_AMAROK_MANAGER_H

#include "importers/ImporterManager.h"

namespace StatSyncing
{

class AmarokManager : public ImporterManager
{
    Q_PLUGIN_METADATA(IID AmarokPluginFactory_iid FILE "amarok_importer-amarok.json")
    Q_INTERFACES(Plugins::PluginFactory)
    Q_OBJECT

public:
    AmarokManager();

    QString type() const;
    QString prettyName() const;
    QString description() const;
    QIcon icon() const;
    ProviderConfigWidget *configWidget( const QVariantMap &config );

protected:
    virtual ImporterProviderPtr newInstance( const QVariantMap &config );
};

} // namespace StatSyncing

#endif // STATSYNCING_AMAROK_MANAGER_H
