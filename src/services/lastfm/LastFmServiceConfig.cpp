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

#include "App.h"
#include "core/support/Debug.h"

#include <KWallet/Wallet>
#include <KDialog>
#include <QLabel>

LastFmServiceConfig::LastFmServiceConfig()
    : m_askDiag( 0 )
    , m_wallet( 0 )
{
    KConfigGroup config = KGlobal::config()->group( configSectionName() );


    // we only want to load the wallet if the user has enabled features that require a user/pass
    bool scrobble = config.readEntry( "scrobble", false );
    bool fetch_sim = config.readEntry( "fetchSimilar", false );

    if( scrobble || fetch_sim ) // if either of these are true we need the wallet.
    {
        // open wallet unless explicitly told not to
        if( !( config.readEntry( "ignoreWallet", QString() ) == "yes" ) ) {
            m_wallet = KWallet::Wallet::openWallet( KWallet::Wallet::NetworkWallet(), 0, KWallet::Wallet::Synchronous );
        }
    }
    load();
}


LastFmServiceConfig::~LastFmServiceConfig()
{
    DEBUG_BLOCK

    if( m_askDiag )
        m_askDiag->deleteLater();
    if( m_wallet )
        m_wallet->deleteLater();
}


void
LastFmServiceConfig::load()
{
    KConfigGroup config = KGlobal::config()->group( configSectionName() );

    if( m_wallet )
    {
        if( !m_wallet->hasFolder( "Amarok" ) )
            m_wallet->createFolder( "Amarok" );
        // do a one-time transfer
        // can remove at some point in the future, post-2.2
        m_wallet->setFolder( "Amarok" );

        if( m_wallet->readPassword( "lastfm_password", m_password ) > 0 )
            debug() << "Failed to read lastfm password from kwallet!";
        QByteArray rawUsername;
        if( m_wallet->readEntry( "lastfm_username", rawUsername ) > 0 )
            debug() << "failed to read last.fm username from kwallet.. :(";
        else
            m_username = QString::fromUtf8( rawUsername );
    }
    else if( config.readEntry( "ignoreWallet", QString() ) != "no" )
    {
        m_username = config.readEntry( "username", QString() );
        m_password = config.readEntry( "password", QString() );
    }

    m_sessionKey = config.readEntry( "sessionKey", QString() );
    m_scrobble = config.readEntry( "scrobble", true );
    m_fetchSimilar = config.readEntry( "fetchSimilar", true );
    m_scrobbleComposer = config.readEntry( "scrobbleComposer", false );
}


void LastFmServiceConfig::save()
{
    debug() << "save config";

    KConfigGroup config = KGlobal::config()->group( configSectionName() );
    config.writeEntry( "sessionKey", m_sessionKey );
    config.writeEntry( "scrobble", m_scrobble );
    config.writeEntry( "fetchSimilar", m_fetchSimilar );
    config.writeEntry( "scrobbleComposer", m_scrobbleComposer );

    if ( !m_wallet && config.readEntry( "ignoreWallet", QString() ) != "yes" )
        askAboutMissingKWallet();

    if( m_wallet )
    {
        m_wallet->setFolder( "Amarok" );
        if( m_wallet->writePassword( "lastfm_password", m_password ) > 0 )
            debug() << "Failed to save last.fm pw to kwallet!";
        if( m_wallet->writeEntry( "lastfm_username", m_username.toUtf8() ) > 0 )
            debug() << "Failed to save last.fm username to kwallet!";
    }
    else if( config.readEntry( "ignoreWallet", QString() ) == "yes" )
    {
        config.writeEntry( "username", m_username );
        config.writeEntry( "password", m_password );
    }
    else
    {
        debug() << "Could not access the wallet to save the last.fm credentials";
    }
    config.sync();
}

void
LastFmServiceConfig::clearSessionKey()
{
    setSessionKey( QString() );
    KConfigGroup config = KGlobal::config()->group( configSectionName() );
    config.writeEntry( "sessionKey", m_sessionKey );
}

void
LastFmServiceConfig::askAboutMissingKWallet()
{
    if ( !m_askDiag )
    {
        m_askDiag = new KDialog( 0 );
        m_askDiag->setCaption( i18n( "Last.fm credentials" ) );
        m_askDiag->setMainWidget( new QLabel( i18n( "No running KWallet found. Would you like Amarok to save your Last.fm credentials in plaintext?" ), m_askDiag ) );
        m_askDiag->setButtons( KDialog::Yes | KDialog::No );
        m_askDiag->setModal( true );

        connect( m_askDiag, SIGNAL( yesClicked() ), this, SLOT( textDialogYes() ) );
        connect( m_askDiag, SIGNAL( noClicked() ), this, SLOT( textDialogNo() ) );
    }
    m_askDiag->exec();
}


void
LastFmServiceConfig::reset()
{
    debug() << "reset config";
    m_username = "";
    m_password = "";
    m_sessionKey = "";
    m_scrobble = false;
    m_fetchSimilar = false;
    m_scrobbleComposer = false;
}


void
LastFmServiceConfig::textDialogYes() //SLOT
{
    DEBUG_BLOCK

    KConfigGroup config = KGlobal::config()->group( configSectionName() );
    config.writeEntry( "ignoreWallet", "yes" );
    config.sync();
}


void
LastFmServiceConfig::textDialogNo() //SLOT
{
    DEBUG_BLOCK

    KConfigGroup config = KGlobal::config()->group( configSectionName() );
    config.writeEntry( "ignoreWallet", "no" );
    config.sync();
}

#include "LastFmServiceConfig.moc"
