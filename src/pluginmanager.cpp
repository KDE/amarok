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

#include <qfile.h>
#include <qstring.h>

#include <kdebug.h>
#include <klibloader.h>
#include <klocale.h>
#include <kmessagebox.h>

using namespace std;
using amaroK::Plugin;


static inline kdbgstream
debug()
{
    return kdbgstream( "[PluginManager] ", 0, 0 );
}

vector<PluginManager::StoreItem>
PluginManager::m_store;


/////////////////////////////////////////////////////////////////////////////////////
// PUBLIC INTERFACE
/////////////////////////////////////////////////////////////////////////////////////

KTrader::OfferList
PluginManager::query( const QString& constraint )
{
    // Add versioning constraint
    QString
    str  = "[X-KDE-amaroK-framework-version] == ";
    str += QString::number( FrameworkVersion );
    str += " and ";
    str += constraint;
    str += " and ";
    str += "[X-KDE-amaroK-rank] > 0";

    debug() << "Plugin trader constraint: " << str << endl;

    return KTrader::self()->query( "amaroK/Plugin", str );
}


Plugin*
PluginManager::createFromQuery( const QString &constraint )
{
    KTrader::OfferList offers = query( constraint );

    if ( offers.isEmpty() ) {
        kdWarning() << k_funcinfo << "No matching plugin found.\n";
        return 0;
    }

    // Select plugin with highest rank
    int rank = 0;
    uint current = 0;
    for ( uint i = 0; i < offers.count(); i++ ) {
        if ( offers[i]->property( "X-KDE-amaroK-rank" ).toInt() > rank )
            current = i;
    }

    return createFromService( offers[current] );
}


Plugin*
PluginManager::createFromService( const KService::Ptr service )
{
    debug() << "Trying to load: " << service->library() << endl;

    //get the library loader instance
    KLibLoader *loader = KLibLoader::self();
    //try to load the specified library
    KLibrary *lib = loader->globalLibrary( QFile::encodeName( service->library() ) );

    if ( !lib ) {
        KMessageBox::error( 0, i18n( "<p>KLibLoader could not load the plugin:<br/><i>%1</i></p>"
                                     "<p>Error message:<br/><i>%2</i></p>" )
                               .arg( service->library() )
                               .arg( loader->lastErrorMessage() ) );
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
    item.library = lib;
    item.service = service;
    m_store.push_back( item );

    dump( service );
    return plugin;
}


void
PluginManager::unload( Plugin* plugin )
{
    kdDebug() << k_funcinfo << endl;

    vector<StoreItem>::iterator iter = lookupPlugin( plugin );

    if ( iter != m_store.end() ) {
        delete (*iter).plugin;
        debug() << "Unloading library: "<< (*iter).service->library() << endl;
        (*iter).library->unload();

        m_store.erase( iter );
    }
    else
        kdWarning() << k_funcinfo << "Could not unload plugin (not found in store).\n";
}


KService::Ptr
PluginManager::getService( const Plugin* plugin )
{
    if ( !plugin ) {
        kdWarning() << k_funcinfo << "pointer == NULL\n";
        return 0;
    }

    //search plugin in store
    vector<StoreItem>::const_iterator iter = lookupPlugin( plugin );

    if ( iter == m_store.end() )
        kdWarning() << k_funcinfo << "Plugin not found in store.\n";

    return (*iter).service;
}


void
PluginManager::showAbout( const QString &constraint )
{
    KTrader::OfferList offers = query( constraint );
    KService::Ptr s = offers.front();

    const QString body = "<tr><td>%1</td><td>%2</td></tr>";

    QString str  = "<html><body><table width=\"100%\" border=\"1\">";

    str += body.arg( i18n( "Name" ),                s->name() );
    str += body.arg( i18n( "Library" ),             s->library() );
    str += body.arg( i18n( "Authors" ),             s->property( "X-KDE-amaroK-authors" ).toStringList().join( "\n" ) );
    str += body.arg( i18n( "Email" ),               s->property( "X-KDE-amaroK-email" ).toStringList().join( "\n" ) );
    str += body.arg( i18n( "Version" ),             s->property( "X-KDE-amaroK-version" ).toString() );
    str += body.arg( i18n( "Framework Version" ),   s->property( "X-KDE-amaroK-framework-version" ).toString() );

    str += "</table></body></html>";

    KMessageBox::information( 0, str, i18n( "Plugin Information" ) );
}


void
PluginManager::dump( const KService::Ptr service )
{
    kdbgstream d( 0, 0 );

    d << endl
      << "PluginManager Service Info:\n"
      << "---------------------------\n"
      << "name                          : " << service->name() << endl
      << "library                       : " << service->library() << endl
      << "desktopEntryPath              : " << service->desktopEntryPath() << endl
      << "X-KDE-amaroK-plugintype       : " << service->property( "X-KDE-amaroK-plugintype" ).toString() << endl
      << "X-KDE-amaroK-name             : " << service->property( "X-KDE-amaroK-name" ).toString() << endl
      << "X-KDE-amaroK-authors          : " << service->property( "X-KDE-amaroK-authors" ).toStringList() << endl
      << "X-KDE-amaroK-rank             : " << service->property( "X-KDE-amaroK-rank" ).toString() << endl
      << "X-KDE-amaroK-version          : " << service->property( "X-KDE-amaroK-version" ).toString() << endl
      << "X-KDE-amaroK-framework-version: " << service->property( "X-KDE-amaroK-framework-version" ).toString() << endl
      << endl;
}


/////////////////////////////////////////////////////////////////////////////////////
// PRIVATE INTERFACE
/////////////////////////////////////////////////////////////////////////////////////

vector<PluginManager::StoreItem>::iterator
PluginManager::lookupPlugin( const Plugin* plugin )
{
    vector<StoreItem>::iterator iter;

    //search plugin pointer in store
    for ( iter = m_store.begin(); iter != m_store.end(); ++iter ) {
        if ( (*iter).plugin == plugin )
            break;
    }

    return iter;
}


