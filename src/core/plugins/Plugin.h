/****************************************************************************************
 * Copyright (c) 2004 Mark Kretschmann <kretschmann@kde.org>                            *
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
 
#ifndef AMAROK_PLUGIN_H
#define AMAROK_PLUGIN_H

#include <config-amarok.h>  
#include "shared/amarok_export.h"

#define AMAROK_EXPORT_PLUGIN( classname ) \
    extern "C" { \
          KDE_EXPORT Plugins::Plugin* create_plugin() { return new classname; } \
    }

#include <QMap>
#include <QString>

class QWidget;

namespace Plugins
{
    /** Bump this number whenever the plugin framework gets incompatible with older versions */
    static const int PluginFrameworkVersion = 55;

    class PluginConfig;

    class AMAROK_EXPORT Plugin
    {
        public:
            virtual ~Plugin();

            /**
             * TODO @param parent you must parent the widget to parent
             * @return the configure widget for your plugin, create it on the heap!
             */
             //TODO rename configureWidget( QWidget *parent )
            virtual PluginConfig* configure() const { return 0; }

            void addPluginProperty( const QString& key, const QString& value );
            QString pluginProperty( const QString& key );
            bool hasPluginProperty( const QString& key );

        protected:
            Plugin();

        private:
            QMap<QString, QString> m_properties;
    };

} //namespace Plugins


#endif /* AMAROK_PLUGIN_H */


