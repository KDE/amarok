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

#include <QByteArray>

#include <SecKeychain.h>
#include <SecKeychainItem.h>

CredentialStorage::Status
CredentialStorage::readEntry( const QString &folder, const QString &key, QString &value )
{
    DEBUG_BLOCK
    debug() << "reading credential with " << folder << " and " << key;
    QByteArray serviceName;
    if( folder.compare( "amarok", Qt::CaseInsensitive ) == 0 )
        serviceName = QString( "org.kde.amarok" ).toUtf8();
    else
        serviceName = folder.toUtf8();
    QByteArray accountName = key.toUtf8();
    UInt32 passwordLength;
    void *password = 0;
    void **passwordPointer = &password;

    OSStatus res = SecKeychainFindGenericPassword(
            NULL, //use default keychain
            serviceName.length(),
            serviceName,
            accountName.length(),
            accountName,
            &passwordLength,
            passwordPointer,
            NULL //itemRef, we do not care about that
           );
    if( res == 0 && passwordPointer )
    {
        debug() << "read credential successfully";
        QString password = QString::fromUtf8( static_cast<char*>( *passwordPointer ) );
        SecKeychainItemFreeContent( NULL, *passwordPointer );
        value = password;
        return CredentialStorage::Success;
    }
    else
    {
        debug() << "reading credential failed: " << res;
        if( passwordPointer )
        {
            debug() << "credential password is not null";
            SecKeychainItemFreeContent( NULL, *passwordPointer );
        }
        else
        {
            debug() << "credential password pointer is null";
        }
        return CredentialStorage::Error;
    }
}

CredentialStorage::Status
CredentialStorage::readPassword( const QString &folder, const QString &key, QString &password )
{
    return readEntry( folder, key, password );
}

CredentialStorage::Status
CredentialStorage::writeEntry( const QString &folder, const QString &key, const QString &value )
{
    DEBUG_BLOCK
    QByteArray serviceName;
    if( folder.compare( "amarok", Qt::CaseInsensitive ) == 0 )
        serviceName = QString( "org.kde.amarok" ).toUtf8();
    else
        serviceName = folder.toUtf8();
    QByteArray accountName = key.toUtf8();
    QByteArray password = value.toUtf8();
    OSStatus res = SecKeychainAddGenericPassword(
            NULL, //use default keychain
            serviceName.length(),
            serviceName,
            accountName.length(),
            accountName,
            password.length(),
            password,
            NULL //itemRef, we do not care about that
            );
    //TODO: overwrite existing entries!
    if( res == 0 )
    {
        debug() << "writing credential succeeded";
        return CredentialStorage::Success;
    }
    else if( res == errSecDuplicateItem )
    {
        debug() << "item already exists";
    }
    else
    {
        debug() << "writing credential failed: " << res;
        return CredentialStorage::Error;
    }
}

CredentialStorage::Status
CredentialStorage::writePassword( const QString &folder, const QString &key, const QString &password )
{
    return writeEntry( folder, key, password );
}
