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

#ifndef CREDENTIALSTORAGE_H
#define CREDENTIALSTORAGE_H

#include "amarok_export.h"

#include <QString>

namespace CredentialStorage
{
    enum Status
    {
        Success,
        DoesNotExist,
        Error
    };

    AMAROK_EXPORT Status readEntry( const QString &folder, const QString &key, QString &value );
    AMAROK_EXPORT Status writeEntry( const QString &folder, const QString &key, const QString &value );
    AMAROK_EXPORT Status writePassword( const QString &folder, const QString &key, const QString &password );
    AMAROK_EXPORT Status readPassword( const QString &folder, const QString &key, QString &password );
}

#endif
