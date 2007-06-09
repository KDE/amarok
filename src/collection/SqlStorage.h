/*
   Copyright (C) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#ifndef AMAROK_SQLSTORAGE_H
#define AMAROK_SQLSTORAGE_H

#include "amarok_export.h"

#include <QString>
#include <QStringList>

class AMAROK_EXPORT SqlStorage
{
public:
    SqlStorage() {}
    virtual ~SqlStorage() {} 

    virtual int sqlDatabasePriority() const = 0;

    virtual QString type() const = 0;

    virtual QString escape( QString text ) const = 0;

    virtual QStringList query( const QString &query ) = 0;
    virtual int insert( const QString &statement, const QString &table ) = 0;

    virtual QString boolTrue() const = 0;
    virtual QString boolFalse() const = 0;

    virtual QString textColumnType( int length = 255 ) const = 0;
    virtual QString exactTextColumnType( int length = 1024 ) const = 0;
    virtual QString longTextColumnType() const = 0;
    virtual QString randomFunc() const = 0;

};

#endif
