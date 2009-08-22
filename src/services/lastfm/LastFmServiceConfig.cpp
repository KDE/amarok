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

#include <KWallet/Wallet>

LastFmServiceConfig::LastFmServiceConfig()
{
    load();
}


void
LastFmServiceConfig::load()
{
    debug() << "load config";
    KConfigGroup config = KGlobal::config()->group( configSectionName() );
    // delete info from kconfig, as a safety measure
    
    // check the wallet if it exists
    KWallet::Wallet* wallet = KWallet::Wallet::openWallet( KWallet::Wallet::NetworkWallet(), 0, KWallet::Wallet::Synchronous );
    if( wallet )
    {
        if( !wallet->hasFolder( "Amarok" ) )
            wallet->createFolder( "Amarok" );
        // do a one-time transfer
        // can remove at some point in the future, post-2.2
        wallet->setFolder( "Amarok" );
        if( config.hasKey( "password" ) )
        {
            wallet->writePassword( "lastfm_password", config.readEntry( "password" ) );
            config.deleteEntry( "password" );
        }
        if( config.hasKey( "username" ) )
        {
            wallet->writeEntry( "lastfm_username", config.readEntry( "username" ).toUtf8() );
            config.deleteEntry( "username" );
        }
        
        if( wallet->readPassword( "lastfm_password", m_password ) > 0 ) 
            debug() << "Failed to read lastfm password from kwallet!";
        QByteArray rawUsername;
        if( wallet->readEntry( "lastfm_username", rawUsername ) > 0 )
            debug() << "failed to read last.fm username from kwallet.. :(";
        else
            m_username = QString::fromUtf8( rawUsername );
    }
    delete wallet;
    
    m_sessionKey = config.readEntry( "sessionKey", QString() );
    m_scrobble = config.readEntry( "scrobble", true );
    m_fetchSimilar = config.readEntry( "fetchSimilar", true );
}


void LastFmServiceConfig::save()
{
    debug() << "save config";

    KWallet::Wallet* wallet = KWallet::Wallet::openWallet( KWallet::Wallet::NetworkWallet(), 0, KWallet::Wallet::Synchronous );
    
    if( wallet )
    {
        wallet->setFolder( "Amarok" );
        if( wallet->writePassword( "lastfm_password", m_password ) > 0 )
            debug() << "Failed to save last.fm pw to kwallet!";
        if( wallet->writeEntry( "lastfm_username", m_username.toUtf8() ) > 0 )
            debug() << "Failed to save last.fm username to kwallet!";
    }
    delete wallet;
    
    KConfigGroup config = KGlobal::config()->group( configSectionName() );
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
