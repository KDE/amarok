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
#include "RadioAdapter.h"
#include "ScrobblerAdapter.h"


AMAROK_EXPORT_PLUGIN( LastFmServiceFactory )


void 
LastFmServiceFactory::init()
{
    LastFmServiceConfig config;

    ServiceBase* service = new LastFmService( "Last.fm", config.username(), UnicornUtils::md5Digest( config.password().toUtf8() ), config.scrobble(), config.fetchSimilar() );
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


LastFmService::LastFmService( const QString &name, const QString &username, const QString &password, bool scrobble, bool fetchSimilar )
    : ServiceBase( name )
{
    setShortDescription(  i18n( "Last.fm: The social music revolution." ) );
    setIcon( KIcon( "amarok_audioscrobbler" ) );
    m_scrobbler = scrobble ? new ScrobblerAdapter( this, username, password ) : 0;
    m_radio = new RadioAdapter( this, username, password );

    Q_ASSERT( ms_service == 0 );
    ms_service = this;
}


LastFmService::~LastFmService()
{
    ms_service = 0;
}


void
LastFmService::polish()
{
}


LastFmService *LastFmService::ms_service = 0;


namespace The
{
    LastFmService *lastFmService()
    {
        return LastFmService::ms_service;
    }
}
