/****************************************************************************************
 * Copyright (c) 2012 Ryan Feng <odayfans@gmail.com>                                    *
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
#include "core/support/Debug.h"
#include <sys/utsname.h>
#include <KWallet/Wallet>
#include <KMessageBox>
#include <KConfig>
#include <KConfigGroup>
#include <KStandardDirs>
#include "App.h"
#include "SpotifyConfig.h"

const QString SpotifyConfig::m_resolverDownloadUrl = "http://ofan.me/";

SpotifyConfig::SpotifyConfig()
: m_username ()
, m_password ()
, m_apikey ()
, m_resolverPath ()
, m_highQuality( false )
, m_wallet ( 0 )
{
    DEBUG_BLOCK

    m_wallet = KWallet::Wallet::openWallet( KWallet::Wallet::NetworkWallet(), 0, KWallet::Wallet::Synchronous );
}

SpotifyConfig::~SpotifyConfig()
{
    DEBUG_BLOCK
    if( m_wallet )
    {
        m_wallet->deleteLater();
    }
}

void
SpotifyConfig::load()
{
    DEBUG_BLOCK
    debug() << "Loading Spotify config...";

    KConfigGroup config = KGlobal::config()->group( configSectionName() );

    if( m_wallet )
    {
        if( !m_wallet->hasFolder( "Amarok" ) )
        {
            m_wallet->createFolder( "Amarok" );
        }

        m_wallet->setFolder( "Amarok" );
        if( m_wallet->readPassword( "spotify_password" , m_password ) > 0 )
        {
            warning() << "Cannot get Spotify password from KWallet!";
        }

        QByteArray resolverPath;
        if( m_wallet->readEntry( "spotify_resolver", resolverPath ) > 0 )
        {
            warning() << "Cannot get Spotify resolver path from KWallet!";
        }
        else
        {
            m_resolverPath = resolverPath;
        }

        QByteArray rawUsername;
        if( m_wallet->readEntry( "spotify_username", rawUsername ) > 0 )
        {
            warning() << "Cannot get Spotify username from KWallet!";
        }
        else
        {
            m_username = QString::fromUtf8( rawUsername );
        }

        QByteArray base64_apikey;
        if( m_wallet->readEntry( "spotify_apikey", base64_apikey ) > 0 )
        {
            debug() << "Cannot get Spotify apikey from KWallet.";
        }
        else
        {
            m_apikey = QByteArray::fromBase64( base64_apikey );
        }

    }
    else
    {
        m_username = config.readEntry( "username", QString() );
        m_resolverPath = config.readEntry( "resolver", QString() );
        m_password = QByteArray::fromBase64( config.readEntry( "password", QString() ).toLocal8Bit() );
        m_apikey = QByteArray::fromBase64( config.readEntry( "apikey", QString() ).toLocal8Bit() );
    }

    m_highQuality = config.readEntry( "highquality", false );
}

void
SpotifyConfig::save()
{
    DEBUG_BLOCK
    debug() << "Saving Spotify config...";

    KConfigGroup config = KGlobal::config()->group( configSectionName() );

    config.writeEntry( "highquality", m_highQuality );

    if( !m_wallet )
    {
        // KWallet not loaded, tell user that we won't save the password
        int result = KMessageBox::questionYesNo( (QWidget*)this,
                i18n( "Cannot find KWallet, credentials will be saved in plaintext, continue?" ),
                i18n( "Spotify credentials" ) );

        if( result == KMessageBox::Cancel )
        {
            return;
        }

        QByteArray base64_password;
        QByteArray base64_apikey;
        base64_password.append( m_password.toLocal8Bit() );
        base64_apikey.append( m_apikey );

        config.writeEntry( "username", m_username );
        config.writeEntry( "password", base64_password.toBase64() );
        config.writeEntry( "apikey", base64_apikey.toBase64() );
        config.writeEntry( "resolver", m_resolverPath );

        config.sync();
    }
    else
    {
        if( m_wallet->writePassword( "spotify_password", m_password ) > 0 )
        {
            warning() << "Failed to save Spotify password to KWallet.";
        }

        if( m_wallet->writeEntry( "spotify_username", m_username.toUtf8() ) > 0 )
        {
            warning() << "Falied to save Spotify username to KWallet.";
        }

        if( m_wallet->writeEntry( "spotify_apikey", m_apikey.toBase64() ) > 0 )
        {
            warning() << "Failed to save Spotify API key to KWallet.";
        }

        if( m_wallet->writeEntry( "spotify_resolver", QByteArray( m_resolverPath.toLocal8Bit() ) ) > 0 )
        {
            warning() << "Failed to save Spotify resolver path to KWallet.";
        }
    }
}

void
SpotifyConfig::reset()
{
    DEBUG_BLOCK
    warning() << "Reset Spotify config";
    m_username = "";
    m_password = "";
    // Use the the API key embedded in Spotify resolver
    m_apikey = "";
    m_resolverPath = KStandardDirs::locateLocal( "exe", resolverName() );
    debug() << "Resolver path: " << m_resolverPath;
}

const QString
SpotifyConfig::resolverName()
{
    utsname buf;
    int res = uname( &buf );
    QString name = "spotify_resolver_";
    if( !res )
        name.append( buf.machine );

    return name;
}

#include "SpotifyConfig.moc"
