/***************************************************************************
begin                : 2004/03/12
copyright            : (C) Mark Kretschmann
email                : markey@web.de
***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include <vector>

#include <qstring.h>
#include <qstringlist.h>

class Plugin;
using namespace std;


class PluginManager
{
    public:
        struct PluginInfo {
            QString specfile;
            QString filename;
            QString author;
            QString license;
            QString type;
            QString site;
            QString email;
            QString name;
            QString comment;
            QStringList require;
        };

        struct StoreItem {
            Plugin*    pointer;
            PluginInfo info;
        };
        
        static vector<PluginInfo>        available   ();
        
        /** 
         * Find all existing plugins of a specified type 
         * @param type    list all plugins of this type (e.g. "engine")  
         * @return        list of all plugin names of the correct type
         */
        static QStringList               available   ( const QString& type );
              
        /** 
         * Read plugin info from disk 
         * @param name    name of plugin (e.g. "artsengine")  
         * @return        PluginInfo struc
         */
        static PluginInfo                getInfo     ( const QString& name );
        
        /** 
         * Look up plugin info for loaded plugin in store 
         * @param pointer pointer to plugin  
         * @return        PluginInfo struc, or empty PluginInfo if not found
         */
        static PluginInfo                getInfo     ( const Plugin* pointer );
        
        /**
         * Load and instantiate plugin 
         * @param name    name of plugin without extension (e.g. "artsengine")  
         * @return        pointer to Plugin, or NULL if error
         */
        static Plugin*                   load        ( const QString& name );
        
        /**
         * Remove library and delete plugin 
         * @param pointer pointer to plugin  
         */
        static void                      unload      ( Plugin* pointer );

    private:
        static vector<StoreItem>         m_store;
};


#endif /* PLUGINMANAGER_H */

