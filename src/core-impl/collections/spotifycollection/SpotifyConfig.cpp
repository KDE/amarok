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
#include "SpotifyConfig.h"
#include "core/support/Debug.h"

#include <sys/utsname.h>
#include <KWallet/Wallet>
#include <KMessageBox>
#include <KConfig>
#include <KConfigGroup>
#include <KStandardDirs>
#include "App.h"

const QString SpotifyConfig::m_resolverDownloadUrl = "http://ofan.me/";

SpotifyConfig::SpotifyConfig()
: m_username ()
, m_password ()
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
    reset();
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

        QByteArray rawUsername;
        if( m_wallet->readEntry( "spotify_username", rawUsername ) > 0 )
        {
            warning() << "Cannot get Spotify username from KWallet!";
        }
        else
        {
            m_username = QString::fromUtf8( rawUsername );
        }

    }
    else
    {
        m_username = config.readEntry( "username", QString() );
        m_resolverPath = config.readEntry( "resolver", QString() );
        m_password = QByteArray::fromBase64( config.readEntry( "password", QString() ).toLocal8Bit() );
    }

    m_highQuality = config.readEntry( "highquality", false );
}

void
SpotifyConfig::save()
{
    DEBUG_BLOCK
    debug() << "Saving Spotify config...";

    KConfigGroup config = KGlobal::config()->group( configSectionName() );


    if( !m_wallet )
    {
        // KWallet not loaded, tell user that we won't save the password
        int result = KMessageBox::questionYesNoCancel( (QWidget*)this,
                i18n( "Cannot find KWallet, credentials will be saved in plaintext, continue?" ),
                i18n( "Spotify credentials" ) );

        if( result == KMessageBox::Cancel )
        {
            // Don't save anything
            return;
        }

        QByteArray base64_password;
        base64_password.append( m_password.toLocal8Bit() );

        config.writeEntry( "username", m_username );

        // Stores password using KConfig if user approved this
        if( result != KMessageBox::No )
            config.writeEntry( "password", base64_password.toBase64() );

        // Set default resolver path
        if( m_resolverPath.isEmpty() )
            m_resolverPath = KStandardDirs::locateLocal( "data",
                               QString("amarok/%1").arg( defaultResolverName() ) );

        config.writeEntry( "resolver", m_resolverPath );

        config.sync();
    }
    else
    {
        // KWallet found

        if( m_wallet->writePassword( "spotify_password", m_password ) > 0 )
        {
            warning() << "Failed to save Spotify password to KWallet.";
        }

        if( m_wallet->writeEntry( "spotify_username", m_username.toUtf8() ) > 0 )
        {
            warning() << "Falied to save Spotify username to KWallet.";
        }
    }

    config.writeEntry( "highquality", m_highQuality );
}

void
SpotifyConfig::reset()
{
    DEBUG_BLOCK
    warning() << "Reset Spotify config";
    m_username = "";
    m_password = "";
    // Use the the API key embedded in Spotify resolver
    m_resolverPath = KStandardDirs::locateLocal( "data",
                       QString("amarok/%1").arg( defaultResolverName() ) );
    debug() << "Resolver path: " << m_resolverPath;
}

const QString
SpotifyConfig::defaultResolverName()
{
    utsname buf;
    int res = uname( &buf );
    QString name = "spotify_resolver_";
    if( !res )
        name.append( buf.machine );

    return name;
}

#include "SpotifyConfig.moc"
