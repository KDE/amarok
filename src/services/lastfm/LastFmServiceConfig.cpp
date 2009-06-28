/****************************************************************************************
 * Copyright (c) 2007 Shane King <kde@dontletsstart.com>                                *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

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
    m_sessionKey = config.readEntry( "sessionKey", QString() );
    m_scrobble = config.readEntry( "scrobble", true );
    m_fetchSimilar = config.readEntry( "fetchSimilar", true );
}


void LastFmServiceConfig::save()
{
    debug() << "save config";
    KConfigGroup config = KGlobal::config()->group( configSectionName() );
    config.writeEntry( "username", m_username );
    config.writeEntry( "password", m_password );
    config.writeEntry( "sessionKey", m_sessionKey );
    config.writeEntry( "scrobble", m_scrobble );
    config.writeEntry( "fetchSimilar", m_fetchSimilar );
}


void LastFmServiceConfig::reset()
{
    debug() << "reset config";
    m_username = "";
    m_password = "";
    m_sessionKey = "";
    m_scrobble = true;
    m_fetchSimilar = true;
}
