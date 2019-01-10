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

#ifndef STATSYNCING_PROVIDER_FACTORY_H
#define STATSYNCING_PROVIDER_FACTORY_H

#include "amarok_export.h"
#include "core/support/PluginFactory.h"

#include <QIcon>

#include <QString>
#include <QVariantMap>

namespace StatSyncing
{
    class Provider;
    typedef QSharedPointer<Provider> ProviderPtr;
    class ProviderConfigWidget;

    /**
     * A class allowing the creation of multiple providers of the same type.
     */
    class AMAROK_EXPORT ProviderFactory : public Plugins::PluginFactory
    {
        Q_OBJECT

        public:
            ProviderFactory();
            virtual ~ProviderFactory();

            /**
             * A string that is unique to this provider factory. It may be used as a key
             * in associative structures.
             */
            virtual QString type() const = 0;

            /**
             * The name of the type of created provider. This name will be displayed
             * in the provider creation dialog.
             */
            virtual QString prettyName() const = 0;

            /**
             * User-visible short localized description. This is the default description
             * of created providers. Default implementation returns an empty string.
             */
            virtual QString description() const;

            /**
             * The icon representing the type of created provider. This icon will be
             * displayed in the provider creation dialog, and is the default icon
             * of created providers.
             */
            virtual QIcon icon() const = 0;

            /**
             * New instance of configuration widget for the provider. Please note that
             * ProviderFactory does *not* retain ownership of this pointer, therefore
             * should always return a new instance.
             */
            virtual ProviderConfigWidget *createConfigWidget() = 0;

            /**
             * Create a new provider instance using configuration stored in @param config
             */
            virtual ProviderPtr createProvider( const QVariantMap &config ) = 0;
    };

} // namespace StatSyncing

#endif // STATSYNCING_PROVIDER_H
