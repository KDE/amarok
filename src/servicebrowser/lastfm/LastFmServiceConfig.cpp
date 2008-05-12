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

#define DEBUG_PREFIX "lastfm"

#include "LastFmServiceConfig.h"
#include "Debug.h"


LastFmServiceConfig::LastFmServiceConfig()
{
    load();
}


void
LastFmServiceConfig::load()
{
    debug() << "load config";
    KConfigGroup config = KGlobal::config()->group( configSectionName() );
    m_username = config.readEntry( "username", QString() );
    m_password = config.readEntry( "password", QString() );
    m_scrobble = config.readEntry( "scrobble", false );
    m_fetchSimilar = config.readEntry( "fetchSimilar", true );
}


void LastFmServiceConfig::save()
{
    debug() << "save config";
    KConfigGroup config = KGlobal::config()->group( configSectionName() );
    config.writeEntry( "username", m_username );
    config.writeEntry( "password", m_password );
    config.writeEntry( "scrobble", m_scrobble );
    config.writeEntry( "fetchSimilar", m_fetchSimilar );
}


void LastFmServiceConfig::reset()
{
    debug() << "reset config";
    m_username = "";
    m_password = "";
    m_scrobble = true;
    m_fetchSimilar = true;
}
