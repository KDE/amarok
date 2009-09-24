/****************************************************************************************
 * Copyright (c) 2009 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#include "CredentialStorage.h"

#include "Debug.h"

#include <QCoreApplication>

#include <KGlobal>
#include <KWallet/Wallet>

class WalletHelper
{
    public:
    WalletHelper();

    ~WalletHelper();

    KWallet::Wallet* getNetworkWallet()
    {
        if( !m_networkWallet )
            m_networkWallet = KWallet::Wallet::openWallet( KWallet::Wallet::NetworkWallet(), 0, KWallet::Wallet::Synchronous );

        return m_networkWallet;
    }

    private:
    KWallet::Wallet *m_networkWallet;
};

K_GLOBAL_STATIC( WalletHelper, s_walletHelper );

WalletHelper::WalletHelper()
    : m_networkWallet( 0 )
{
    qAddPostRoutine( s_walletHelper.destroy );
}

WalletHelper::~WalletHelper()
{
    if( m_networkWallet )
        delete m_networkWallet;
    qRemovePostRoutine( s_walletHelper.destroy );
}

CredentialStorage::Status
CredentialStorage::readEntry( const QString &folder, const QString &key, QString &value )
{
    if( s_walletHelper.isDestroyed() )
    {
        warning() << "Accessing CredentialStorage afer the internal helper has been destroyed";
        return CredentialStorage::Error;
    }
    KWallet::Wallet *wallet = s_walletHelper->getNetworkWallet();
    if( !wallet )
        return CredentialStorage::Error;

    if(!wallet->hasFolder( folder ) )
    {
        value = QString();
        return CredentialStorage::DoesNotExist;
    }
    else
    {
        wallet->setFolder( folder );
        QByteArray ba;
        if( wallet->readEntry( key, ba ) > 0 )
        {
            return CredentialStorage::Error;
        }
        else
        {
            value = QString::fromUtf8( ba );
            return CredentialStorage::Success;
        }
    }
}

CredentialStorage::Status
CredentialStorage::readPassword( const QString &folder, const QString &key, QString &password )
{
    if( s_walletHelper.isDestroyed() )
    {
        warning() << "Accessing CredentialStorage afer the internal helper has been destroyed";
        return CredentialStorage::Error;
    }
    KWallet::Wallet *wallet = s_walletHelper->getNetworkWallet();
    if( !wallet )
        return CredentialStorage::Error;

    if(!wallet->hasFolder( folder ) )
    {
        password.clear();
        return CredentialStorage::DoesNotExist;
    }
    else
    {
        wallet->setFolder( folder );
        if( wallet->readPassword( key, password ) > 0 )
        {
            password.clear();
            return CredentialStorage::Error;
        }
        else
        {
            return CredentialStorage::Success;
        }
    }
}

CredentialStorage::Status
CredentialStorage::writeEntry( const QString &folder, const QString &key, const QString &value )
{
    if( s_walletHelper.isDestroyed() )
    {
        warning() << "Accessing CredentialStorage afer the internal helper has been destroyed";
        return CredentialStorage::Error;
    }
    KWallet::Wallet *wallet = s_walletHelper->getNetworkWallet();
    if( wallet )
    {
        if( !wallet->hasFolder( folder ) )
        {
            if( !wallet->createFolder( folder ) )
                return CredentialStorage::Error;
        }
        wallet->setFolder( folder );
        if( wallet->writeEntry( key, value.toUtf8() ) > 0 )
            return CredentialStorage::Error;
        return CredentialStorage::Success;
    }
    else
        return CredentialStorage::Error;
}

CredentialStorage::Status
CredentialStorage::writePassword( const QString &folder, const QString &key, const QString &password )
{
    if( s_walletHelper.isDestroyed() )
    {
        warning() << "Accessing CredentialStorage afer the internal helper has been destroyed";
        return CredentialStorage::Error;
    }
    KWallet::Wallet *wallet = s_walletHelper->getNetworkWallet();
    if( wallet )
    {
        if( !wallet->hasFolder( folder ) )
        {
            if( !wallet->createFolder( folder ) )
                return CredentialStorage::Error;
        }
        wallet->setFolder( folder );
        if( wallet->writePassword( key, password ) > 0 )
            return CredentialStorage::Error;
        return CredentialStorage::Success;
    }
    else
        return CredentialStorage::Error;
}
