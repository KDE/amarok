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
#include "core/logger/Logger.h"
#include "core/support/Amarok.h"
#include "core/support/Components.h"
#include "core/support/Debug.h"

#include <KLocalizedString>
#include <KWallet>

#include <QMessageBox>
#include <QLabel>
#include <QThread>

QWeakPointer<LastFmServiceConfig> LastFmServiceConfig::s_instance;

LastFmServiceConfigPtr
LastFmServiceConfig::instance()
{
    Q_ASSERT( QThread::currentThread() == QCoreApplication::instance()->thread() );

    LastFmServiceConfigPtr strongRef = s_instance.toStrongRef();
    if( strongRef )
        return strongRef;

    LastFmServiceConfigPtr newStrongRef( new LastFmServiceConfig() );
    s_instance = newStrongRef;
    return newStrongRef;
}

LastFmServiceConfig::LastFmServiceConfig()
    : m_askDiag( nullptr )
    , m_wallet( nullptr )
{
    DEBUG_BLOCK

    KConfigGroup config = Amarok::config( configSectionName() );
    m_sessionKey = config.readEntry( "sessionKey", QString() );
    m_scrobble = config.readEntry( "scrobble", defaultScrobble() );
    m_fetchSimilar = config.readEntry( "fetchSimilar", defaultFetchSimilar() );
    m_scrobbleComposer = config.readEntry( "scrobbleComposer", defaultScrobbleComposer() );
    m_useFancyRatingTags = config.readEntry( "useFancyRatingTags", defaultUseFancyRatingTags() );
    m_announceCorrections = config.readEntry( "announceCorrections", defaultAnnounceCorrections() );
    m_filterByLabel = config.readEntry( "filterByLabel", defaultFilterByLabel() );
    m_filteredLabel = config.readEntry( "filteredLabel", defaultFilteredLabel() );

    if( config.hasKey( "kWalletUsage" ) )
        m_kWalletUsage = KWalletUsage( config.readEntry( "kWalletUsage", int( NoPasswordEnteredYet ) ) );
    else
    {
        // migrate from the old config that used "ignoreWallet" key set to yes/no
        if( config.readEntry( "ignoreWallet", "" ) == QStringLiteral("yes") )
            m_kWalletUsage = PasswordInAscii;
        else if( config.hasKey( "scrobble" ) )
            // assume password was saved in KWallet if the config was once written
            m_kWalletUsage = PasswodInKWallet;
        else
            m_kWalletUsage = NoPasswordEnteredYet; // config not yet saved, assume unused
    }

    switch( m_kWalletUsage )
    {
        case NoPasswordEnteredYet:
            break;
        case PasswodInKWallet:
            openWalletToRead();
            break;
        case PasswordInAscii:
            m_username = config.readEntry( "username", QString() );
            m_password = config.readEntry( "password", QString() );
            break;
    }
}

LastFmServiceConfig::~LastFmServiceConfig()
{
    DEBUG_BLOCK

    if( m_askDiag )
        m_askDiag->deleteLater();
    if( m_wallet )
        m_wallet->deleteLater();
}

void LastFmServiceConfig::save()
{
    KConfigGroup config = Amarok::config( configSectionName() );

    // if username and password is empty, reset to NoPasswordEnteredYet; this enables
    // going from PasswordInAscii to PasswodInKWallet
    if( m_username.isEmpty() && m_password.isEmpty() )
    {
        m_kWalletUsage = NoPasswordEnteredYet;
        config.deleteEntry( "username" ); // prevent possible stray credentials
        config.deleteEntry( "password" );
    }

    config.writeEntry( "sessionKey", m_sessionKey );
    config.writeEntry( "scrobble", m_scrobble );
    config.writeEntry( "fetchSimilar", m_fetchSimilar );
    config.writeEntry( "scrobbleComposer", m_scrobbleComposer );
    config.writeEntry( "useFancyRatingTags", m_useFancyRatingTags );
    config.writeEntry( "announceCorrections", m_announceCorrections );
    config.writeEntry( "kWalletUsage", int( m_kWalletUsage ) );
    config.writeEntry( "filterByLabel", m_filterByLabel );
    config.writeEntry( "filteredLabel", m_filteredLabel );
    config.deleteEntry( "ignoreWallet" ); // remove old settings

    switch( m_kWalletUsage )
    {
        case NoPasswordEnteredYet:
            if( m_username.isEmpty() && m_password.isEmpty() )
                break; // stay in this state
            Q_FALLTHROUGH();
        case PasswodInKWallet:
            openWalletToWrite();
            config.deleteEntry( "username" ); // prevent possible stray credentials
            config.deleteEntry( "password" );
            break;
        case PasswordInAscii:
            config.writeEntry( "username", m_username );
            config.writeEntry( "password", m_password );
            break;
    }

    config.sync();
    Q_EMIT updated();
}

void
LastFmServiceConfig::openWalletToRead()
{
    if( m_wallet && m_wallet->isOpen() )
    {
        slotWalletOpenedToRead( true );
        return;
    }

    if( m_wallet )
        disconnect( m_wallet, nullptr, this, nullptr );
    else
    {
        openWalletAsync();
        if( !m_wallet ) // can happen, see bug 322964
        {
            slotWalletOpenedToRead( false );
            return;
        }
    }
    connect( m_wallet, &KWallet::Wallet::walletOpened, this, &LastFmServiceConfig::slotWalletOpenedToRead );
}

void
LastFmServiceConfig::openWalletToWrite()
{
    if( m_wallet && m_wallet->isOpen() )
    {
        slotWalletOpenedToWrite( true );
        return;
    }

    if( m_wallet )
        disconnect( m_wallet, nullptr, this, nullptr );
    else
    {
        openWalletAsync();
        if( !m_wallet ) // can happen, see bug 322964
        {
            slotWalletOpenedToWrite( false );
            return;
        }
    }
    connect( m_wallet, &KWallet::Wallet::walletOpened, this, &LastFmServiceConfig::slotWalletOpenedToWrite );
}

void
LastFmServiceConfig::openWalletAsync()
{
    Q_ASSERT( !m_wallet );
    using namespace KWallet;
    m_wallet = Wallet::openWallet( Wallet::NetworkWallet(), 0, Wallet::Asynchronous );
}

void
LastFmServiceConfig::prepareOpenedWallet()
{
    if( !m_wallet->hasFolder( QStringLiteral("Amarok") ) )
        m_wallet->createFolder( QStringLiteral("Amarok") );
    m_wallet->setFolder( QStringLiteral("Amarok") );
}

void
LastFmServiceConfig::slotWalletOpenedToRead( bool success )
{
    if( !success )
    {
        warning() << __PRETTY_FUNCTION__ << "failed to open wallet";
        QString message = i18n( "Failed to open KDE Wallet to read Last.fm credentials" );
        Amarok::Logger::longMessage( message, Amarok::Logger::Warning );
        if( m_wallet )
            m_wallet->deleteLater(); // no point in having invalid wallet around
        m_wallet = nullptr;
        return;
    }

    Q_ASSERT( m_wallet );
    prepareOpenedWallet();

    if( m_wallet->readPassword( QStringLiteral("lastfm_password"), m_password ) > 0 )
        warning() << "Failed to read lastfm password from kwallet";
    QByteArray rawUsername;
    if( m_wallet->readEntry( QStringLiteral("lastfm_username"), rawUsername ) > 0 )
        warning() << "Failed to read last.fm username from kwallet";
    else
        m_username = QString::fromUtf8( rawUsername );
    Q_EMIT updated();
}

void
LastFmServiceConfig::slotWalletOpenedToWrite( bool success )
{
    if( !success )
    {
        askAboutMissingKWallet();
        if( m_wallet )
            m_wallet->deleteLater(); // no point in having invalid wallet around
        m_wallet = nullptr;
        return;
    }

    Q_ASSERT( m_wallet );
    prepareOpenedWallet();

    if( m_wallet->writePassword( QStringLiteral("lastfm_password"), m_password ) > 0 )
        warning() << "Failed to save last.fm password to kwallet";
    if( m_wallet->writeEntry( QStringLiteral("lastfm_username"), m_username.toUtf8() ) > 0 )
        warning() << "Failed to save last.fm username to kwallet";

    m_kWalletUsage = PasswodInKWallet;
    KConfigGroup config = Amarok::config( configSectionName() );
    config.writeEntry( "kWalletUsage", int( m_kWalletUsage ) );
    config.sync();
}

void
LastFmServiceConfig::askAboutMissingKWallet()
{
    if ( !m_askDiag )
    {
        m_askDiag = new QMessageBox;
        m_askDiag->setText( i18n( "No running KWallet found." ) );
        m_askDiag->setInformativeText( i18n( "Would you like Amarok to save your Last.fm credentials in plaintext?" ) );
        m_askDiag->setStandardButtons( QMessageBox::Yes | QMessageBox::No );

        connect( m_askDiag, &QDialog::accepted, this, &LastFmServiceConfig::slotStoreCredentialsInAscii );
        // maybe connect SIGNAL(noClicked()) to a message informing the user the password will
        // be forgotten on Amarok restart
    }
    m_askDiag->show();
}

void
LastFmServiceConfig::slotStoreCredentialsInAscii() //SLOT
{
    DEBUG_BLOCK
    m_kWalletUsage = PasswordInAscii;
    save();
}
