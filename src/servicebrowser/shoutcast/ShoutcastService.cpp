/***************************************************************************
 *   Copyright (c) 2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>     *
 *             (c) 2007 Adam Pigg <adam@piggz.co.uk>                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "ShoutcastService.h"

#include "debug.h"
#include "amarok.h"
#include "ContextStatusBar.h"

#include <KTemporaryFile>



using namespace Meta;

AMAROK_EXPORT_PLUGIN( ShoutcastServiceFactory )

void ShoutcastServiceFactory::init()
{
    ServiceBase* service = new ShoutcastService( "Shoutcast.com" );
    emit newService( service );
}

QString ShoutcastServiceFactory::name()
{
    return "Shoutcast.com";
}


KPluginInfo ShoutcastServiceFactory::info()
{
    KPluginInfo pluginInfo(  "amarok_service_shoutcast.desktop", "services" );
    pluginInfo.setConfig( config() );
    return pluginInfo;
}

KConfigGroup ShoutcastServiceFactory::config()
{
    return Amarok::config( "Service_Shoutcast" );
}



ShoutcastService::ShoutcastService( const char *name )
    : ServiceBase( "Shoutcast Directory" )
{
    setObjectName( name );
    setShortDescription( i18n( "The biggest damn list of online radio stations on the net :-)" ) );
    setIcon( KIcon( Amarok::icon( "network-wireless" ) ) );
    showInfo( false );
}


ShoutcastService::~ShoutcastService()
{
}

void ShoutcastService::polish()
{
    DEBUG_BLOCK

    if ( m_polished )
        return;

    m_collection = new ShoutcastServiceCollection();
    QList<int> levels;
    levels << CategoryId::Genre;
    setModel( new SingleCollectionTreeItemModel( m_collection, levels ) );
    m_polished = true;

}


#include "ShoutcastService.moc"





