/****************************************************************************************
 * Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>            *
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

#ifndef AMAROK_SQLSTORAGE_H
#define AMAROK_SQLSTORAGE_H

#include "shared/amarok_export.h"

#include <QMetaType>
#include <QString>
#include <QStringList>

class SqlStorage
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

    /**
        use this type for auto incrementing integer primary keys.
    */
    virtual QString idType() const = 0;

    virtual QString textColumnType( int length = 255 ) const = 0;
    virtual QString exactTextColumnType( int length = 1000 ) const = 0;
    //the below value may have to be decreased even more for different indexes; only time will tell
    virtual QString exactIndexableTextColumnType( int length = 324 ) const = 0;
    virtual QString longTextColumnType() const = 0;
    virtual QString randomFunc() const = 0;

};

Q_DECLARE_METATYPE( SqlStorage * )

#endif
