// Author: Mark Kretschmann (C) Copyright 2004
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
 
#ifndef AMAROK_PLUGIN_H
#define AMAROK_PLUGIN_H

#include <config-amarok.h>  
#include "amarok_export.h"

#define AMAROK_EXPORT_PLUGIN( classname ) \
    extern "C" { \
          KDE_EXPORT Amarok::Plugin* create_plugin() { return new classname; } \
    }

#include <qmap.h>
#include <QString>

class QWidget;

namespace Amarok
{
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

} //namespace Amarok


#endif /* AMAROK_PLUGIN_H */


