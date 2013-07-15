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

#ifndef STATSYNCING_FAST_FORWARD_IMPORTER_FACTORY_H
#define STATSYNCING_FAST_FORWARD_IMPORTER_FACTORY_H

#include "importers/ImporterFactory.h"

namespace StatSyncing
{

class FastForwardFactory : public ImporterFactory
{
    Q_OBJECT

public:
    FastForwardFactory( QObject *parent, const QVariantList &args );
    ~FastForwardFactory();

    QString id() const;
    KPluginInfo info() const;
    QString prettyName() const;
    QString description() const;
    KIcon icon() const;
    ProviderPtr createProvider( const QVariantMap &config );
    ProviderConfigWidget *getConfigWidget( const QVariantMap &config );
};

} // namespace StatSyncing

#endif // STATSYNCING_FAST_FORWARD_IMPORTER_FACTORY_H
