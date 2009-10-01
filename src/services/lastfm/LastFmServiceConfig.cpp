/****************************************************************************************
 * Copyright (c) 2007 Shane King <kde@dontletsstart.com>                                *
 * Copyright (c) 2009 Leo Franchi <lfranchi@kde.org>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#define DEBUG_PREFIX "lastfm"

#include "LastFmServiceConfig.h"

#include "CredentialStorage.h"
#include "Debug.h"

#include <MainWindow.h>
#include <QLabel>

LastFmServiceConfig::LastFmServiceConfig()
{
    KConfigGroup config = KGlobal::config()->group( configSectionName() );
    load();
}


LastFmServiceConfig::~LastFmServiceConfig()
{
}


void
LastFmServiceConfig::load()
{
    KConfigGroup config = KGlobal::config()->group( configSectionName() );
    // delete info from kconfig, as a safety measure

    // do a one-time transfer
    // can remove at some point in the future, post-2.2
    if( config.hasKey( "password" ) )
    {
        CredentialStorage::writePassword( "Amarok", "lastfm_password", config.readEntry( "password" ) );
        config.deleteEntry( "password" );
    }
    if( config.hasKey( "username" ) )
    {
        CredentialStorage::writeEntry( "Amarok", "lastfm_username", config.readEntry( "username" ) );
        config.deleteEntry( "username" );
    }
    CredentialStorage::readEntry( "Amarok", "lastfm_username", m_username );
    CredentialStorage::readPassword( "Amarok", "lastfm_password", m_password );
    
    m_sessionKey = config.readEntry( "sessionKey", QString() );
    m_scrobble = config.readEntry( "scrobble", true );
    m_fetchSimilar = config.readEntry( "fetchSimilar", true );
}


void LastFmServiceConfig::save()
{
    debug() << "save config";

    KConfigGroup config = KGlobal::config()->group( configSectionName() );
    config.writeEntry( "sessionKey", m_sessionKey );
    config.writeEntry( "scrobble", m_scrobble );
    config.writeEntry( "fetchSimilar", m_fetchSimilar );

    CredentialStorage::writeEntry( "Amarok", "lastfm_username", m_username );
    CredentialStorage::writePassword( "Amarok", "lastfm_password", m_password );
}

void
LastFmServiceConfig::reset()
{
    debug() << "reset config";
    m_username = "";
    m_password = "";
    m_sessionKey = "";
    m_scrobble = true;
    m_fetchSimilar = true;
}

#include "LastFmServiceConfig.moc"
