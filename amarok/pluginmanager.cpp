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

#include "pluginmanager.h"

#include <vector>

#include <qfile.h>
#include <qstring.h>
#include <qstringlist.h>

#include <kdebug.h>
#include <kglobal.h>
#include <klibloader.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>
#include <kurl.h>

using namespace std;


vector<PluginManager::StoreItem>
    PluginManager::m_store;


vector<PluginManager::PluginInfo>
    PluginManager::available()
{
    vector<PluginInfo> items;
    QStringList files = KGlobal::dirs()->findAllResources( "appdata", "*.plugin", false, true );

    for ( QStringList::Iterator i = files.begin(); i != files.end(); ++i )
        items.push_back( getInfo( *i ) );

    return items;
}


QStringList
    PluginManager::available( const QString &type )
{
    vector<PluginInfo> pool = available();
    QStringList result;
    
    for ( int i = 0; i < pool.size(); i++ ) {
        if ( pool[i].type == type )
            result.append( pool[i].filename );
    }
     
    return result;                   
}


PluginManager::PluginInfo
    PluginManager::getInfo( const QString &name )
{
    PluginInfo info;
    QString path = ( name[0] == '/' ) ? name : KGlobal::dirs()->findResource( "appdata", name );
    
    if ( !QFile::exists( path ) )
        return info;
    
    KSimpleConfig file( path );
    
    if ( name.find('/') >= 0 )
        info.specfile = KURL( name ).fileName();
    else
        info.specfile = name;
    
    info.filename    = file.readEntry    ( "Filename" );
    info.author      = file.readEntry    ( "Author" );
    info.site        = file.readEntry    ( "Site" );
    info.email       = file.readEntry    ( "Email" );
    info.type        = file.readEntry    ( "Type" );
    info.name        = file.readEntry    ( "Name" );
    info.comment     = file.readEntry    ( "Comment" );
    info.require     = file.readListEntry( "Require" );
    info.license     = file.readEntry    ( "License" );
    
    return info;
}


PluginManager::PluginInfo
    PluginManager::getInfo( const void* pointer )
{
    PluginInfo info;
    
    if ( !pointer )
        return info;

    //search plugin in store
    for ( vector<StoreItem>::iterator it = m_store.begin(); it != m_store.end(); ++it ) {
        if ( (*it).pointer == pointer )
            info = (*it).info;
    }
                
    return info;
}


void*
    PluginManager::load( const QString& name )
{
    // get the library loader instance
    KLibLoader *loader = KLibLoader::self();

    QString filename = KGlobal::dirs()->findResource( "module", name + ".la" );
    KLibrary *lib = loader->library( QFile::encodeName( filename ) );

    if ( !lib ) {
        kdWarning() << k_funcinfo << "lib == NULL\n";
        return 0;
    }

    void* create = lib->symbol( "create_plugin" );

    if ( !create ) {
        kdWarning() << k_funcinfo << "create == NULL\n";
        return 0;
    }

    void* (*plugInStart)();
    plugInStart = ( void* (*)() ) create;
    void* pointer = plugInStart();
    
    //put plugin into store
    StoreItem item;
    item.pointer = pointer;
    item.info    = getInfo( name );
    m_store.push_back( item );
    
    return plugInStart();
}


void
    PluginManager::unload( void* pointer )
{
    vector<StoreItem>::iterator it;
    
    //determine if store contains this plugin
    for ( it = m_store.begin(); it != m_store.end(); ++it ) {
        if ( (*it).pointer == pointer )
            break;
    }

    if ( it != m_store.end() ) {
        delete (*it).pointer;
        
        KLibLoader *loader = KLibLoader::self();
        loader->unloadLibrary( QFile::encodeName( (*it).info.filename ) );
        m_store.erase( it );
    }
}

