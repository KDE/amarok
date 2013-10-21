/****************************************************************************************
 * Copyright (c) 2010 Rick W. Chen <stuffcorpse@archlinux.us>                           *
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

#ifndef AMAROK_PLUGINFACTORY_H
#define AMAROK_PLUGINFACTORY_H

#include "core/amarokcore_export.h"

#include <KPluginInfo>

#include <QObject>

namespace Plugins {

class AMAROK_CORE_EXPORT PluginFactory : public QObject
{
    Q_OBJECT
    Q_ENUMS( Type )
    Q_PROPERTY( KPluginInfo info READ info )
    Q_PROPERTY( Type pluginType READ pluginType )

public:
    enum Type { Unknown, Collection, Service, Importer };
    PluginFactory( QObject *parent, const QVariantList &args );
    virtual ~PluginFactory() = 0;

    /**
     * Initialize the service plugin of this type.
     * Reimplemented by subclasses, which must set
     * m_initialized = true when the function has finished.
     * */
    virtual void init() = 0;

    /**
     * Indicates whether or not the plugin has been initialized.
     * @return Initialized or not
     */
    bool isInitialized() const;

    KPluginInfo info() const;
    Type pluginType() const;

protected:
    bool m_initialized;
    KPluginInfo m_info;
    Type m_type;
};

} // namespace Plugins

#endif /* AMAROK_PLUGINFACTORY_H */
