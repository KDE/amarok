/***************************************************************************
 * copyright            : (C) 2007 Shane King <kde@dontletsstart.com>      *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "LastFmService.h"
#include "LastFmServiceConfig.h"


AMAROK_EXPORT_PLUGIN( LastFmServiceFactory )


void 
LastFmServiceFactory::init()
{
    ServiceBase* service = new LastFmService( "Last.fm" );
    emit newService( service );
}


QString 
LastFmServiceFactory::name()
{
    return "Last.fm";
}


KPluginInfo 
LastFmServiceFactory::info()
{
    KPluginInfo pluginInfo(  "amarok_service_lastfm.desktop", "services" );
    pluginInfo.setConfig( config() );
    return pluginInfo;
}


KConfigGroup 
LastFmServiceFactory::config()
{
    return Amarok::config( LastFmServiceConfig::configSectionName() );
}


LastFmService::LastFmService( const QString &name )
    : ServiceBase( name )
{
    setShortDescription(  i18n( "Last.fm: The social music revolution." ) );
    setIcon( KIcon( Amarok::icon( "amarok_audioscrobbler" ) ) );
}


LastFmService::~LastFmService()
{
}


void
LastFmService::polish()
{
}
