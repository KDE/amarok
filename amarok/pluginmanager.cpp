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

#include "plugin.h"
#include "pluginmanager.h"

#include <vector>

#include <qstring.h>

#include <kdebug.h>
#include <klibloader.h>
#include <kurl.h>

using namespace std;


vector<PluginManager::StoreItem>
    PluginManager::m_store;


KTrader::OfferList     
    PluginManager::query( const QString& constraint )
{    
    return KTrader::self()->query( "amaroK/Plugin", constraint );
}    
    
    
Plugin*
    PluginManager::createFromQuery( const QString& constraint )
{
    kdDebug() << k_funcinfo << "constraint string: " << constraint << endl;
    KTrader::OfferList offers = KTrader::self()->query( "amaroK/Plugin", constraint );

    if ( offers.isEmpty() ) {
        kdWarning() << k_funcinfo << "No matching plugin found.\n";                                          
        return 0;
    }
                
    return createFromService( *offers.begin() );    
}

    
Plugin*
    PluginManager::createFromService( const KService::Ptr service )
{
    kdDebug() << k_funcinfo << "Trying to load: " << service->library() << endl;
    
    //get the library loader instance
    KLibLoader *loader = KLibLoader::self();
    //try to load the specified library
    KLibrary *lib = loader->globalLibrary( service->library().latin1() );

    if ( !lib ) {
        kdWarning() << k_funcinfo << "lib == NULL\n";
        return 0;
    }

    //look up address of init function and cast it to pointer-to-function
    Plugin* (*create_plugin)() = ( Plugin* (*)() ) lib->symbol( "create_plugin" );

    if ( !create_plugin ) {
        kdWarning() << k_funcinfo << "create_plugin == NULL\n";
        return 0;
    }

    //create plugin on the heap
    Plugin* plugin = create_plugin();
    
    //put plugin into store
    StoreItem item;
    item.plugin = plugin;
    item.service = service;
    m_store.push_back( item );
    
    dump( service );
    return plugin;
}


void
    PluginManager::unload( Plugin* plugin )
{
    kdDebug() << k_funcinfo << endl;
    
    vector<StoreItem>::iterator it;
    //determine if store contains this plugin
    for ( it = m_store.begin(); it != m_store.end(); ++it ) {
        if ( (*it).plugin == plugin )
            break;
    }

    if ( it != m_store.end() ) {
        delete (*it).plugin;
        KLibLoader *loader = KLibLoader::self();
        loader->unloadLibrary( (*it).service->library().latin1() );
        
        m_store.erase( it );
    }
}


KService::Ptr
    PluginManager::getService( const Plugin* plugin )
{
    kdDebug() << k_funcinfo << endl;
    
    KService::Ptr service;
    
    if ( !plugin ) {
        kdWarning() << k_funcinfo << "pointer == NULL\n";   
        return service;
    }
        
    //search plugin in store
    for ( vector<StoreItem>::iterator it = m_store.begin(); it != m_store.end(); ++it ) {
        if ( (*it).plugin == plugin )
            service = (*it).service;
    }
    
    return service;
}


void    
    PluginManager::dump( const KService::Ptr service )
{
    if ( service ) {
        kdDebug() << endl;
        
        kdDebug() << "PluginManager Service DUMP:\n";
        kdDebug() << "---------------------------\n";
        
        kdDebug() << "name                          : "
                  << service->name()                                                  << endl;
        kdDebug() << "library                       : "
                  << service->library()                                               << endl;
        kdDebug() << "desktopEntryPath              : "
                  << service->desktopEntryPath()                                      << endl;
        kdDebug() << "X-KDE-plugintype              : "
                  << service->property( "X-KDE-amaroK-plugintype" ).toString()        << endl;
        kdDebug() << "X-KDE-amaroK-authors          : "
                  << service->property( "X-KDE-amaroK-authors" ).toStringList()       << endl;
        kdDebug() << "X-KDE-amaroK-version          : "
                  << service->property( "X-KDE-amaroK-version" ).toString()           << endl;
        kdDebug() << "X-KDE-amaroK-framework-version: "
                  << service->property( "X-KDE-amaroK-framework-version" ).toString() << endl;
        
        kdDebug() << endl;
    }    
}

    
