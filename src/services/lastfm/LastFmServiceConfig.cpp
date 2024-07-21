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
#include <KMessageBox>
#include <KWallet>

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
    : m_wallet( nullptr )
{
    DEBUG_BLOCK

    KConfigGroup config = Amarok::config( configSectionName() );
    m_sessionKey = config.readEntry( "sessionKey", QString() );
    m_username = config.readEntry( "username", QString() );
    m_scrobble = config.readEntry( "scrobble", defaultScrobble() );
    m_scrobbleComposer = config.readEntry( "scrobbleComposer", defaultScrobbleComposer() );
    m_useFancyRatingTags = config.readEntry( "useFancyRatingTags", defaultUseFancyRatingTags() );
    m_announceCorrections = config.readEntry( "announceCorrections", defaultAnnounceCorrections() );
    m_filterByLabel = config.readEntry( "filterByLabel", defaultFilterByLabel() );
    m_filteredLabel = config.readEntry( "filteredLabel", defaultFilteredLabel() );

    if( !m_sessionKey.isEmpty() && m_username.isEmpty() && config.hasKey( "kWalletUsage" ) &&
        KWalletUsage( config.readEntry( "kWalletUsage", int( NoPasswordEnteredYet ) ) ) == PasswodInKWallet )
    {
        debug() << "Last.fm seems to be active, but no username in config, trying to migrate from kwallet";
        openWalletToRead(); // migrate away from KWallet to a state that corresponds to token-based authentication
    }
}

LastFmServiceConfig::~LastFmServiceConfig()
{
    DEBUG_BLOCK

    if( m_wallet )
        m_wallet->deleteLater();
}

void LastFmServiceConfig::save()
{
    KConfigGroup config = Amarok::config( configSectionName() );

    config.writeEntry( "username", m_username );
    config.writeEntry( "sessionKey", m_sessionKey );
    config.writeEntry( "scrobble", m_scrobble );
    config.writeEntry( "scrobbleComposer", m_scrobbleComposer );
    config.writeEntry( "useFancyRatingTags", m_useFancyRatingTags );
    config.writeEntry( "announceCorrections", m_announceCorrections );
    config.writeEntry( "filterByLabel", m_filterByLabel );
    config.writeEntry( "filteredLabel", m_filteredLabel );
    if( !m_username.isEmpty() ) // any migration seems to be done successfully, remove old settings
        config.deleteEntry( "kWalletUsage" );
    config.deleteEntry( "ignoreWallet" ); // remove old settings

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
LastFmServiceConfig::openWalletAsync()
{
    Q_ASSERT( !m_wallet );
    using namespace KWallet;
    m_wallet = Wallet::openWallet( Wallet::NetworkWallet(), 0, Wallet::Asynchronous );
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
    m_wallet->setFolder( QStringLiteral("Amarok") );

    QByteArray rawUsername;
    if( m_wallet->readEntry( QStringLiteral("lastfm_username"), rawUsername ) > 0 )
        warning() << "Failed to read last.fm username from kwallet";
    else
        m_username = QString::fromUtf8( rawUsername );
    if( m_username.isEmpty() && !m_sessionKey.isEmpty() )
        KMessageBox::error( nullptr, i18n( "Error migrating Last.fm username from KWallet. Please reconnect your Last.fm account in Plugin Settings." ) );
    else
        save();
    Q_EMIT updated();
}

