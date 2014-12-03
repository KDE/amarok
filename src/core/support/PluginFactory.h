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

/** Baseclass for Amarok plugins.
 *
 *  This class is subclassed for the different type of Amarok plugins.
 *  - CollectionFactory
 *  - ServiceFactory
 *  - ImportFactory
 *  - StorageFactory
 *
 *  Plugins need to set the m_info variables.
 *  Plugins also need to set a valid category in their Desktop file.
 */
class AMAROK_CORE_EXPORT PluginFactory : public QObject
{
    Q_OBJECT
    Q_PROPERTY( KPluginInfo info READ info )

public:
    PluginFactory( QObject *parent, const QVariantList &args );
    virtual ~PluginFactory() = 0;

    /** Initialize the service plugin of this type.
     *
     * Reimplemented by subclasses, which must set
     * m_initialized = true when the function has finished.
     *
     * This function is called by the PluginManager after
     * setting the plugin in the different sub-plugin managers
     * */
    virtual void init() = 0;

    KPluginInfo info() const;

protected:
    bool m_initialized; ///< set after init was called
    KPluginInfo m_info; ///< contains the KPluginInfo returned by info(). Should be set in the constructor
};

} // namespace Plugins


#endif /* AMAROK_PLUGINFACTORY_H */
