/****************************************************************************************
 * Copyright (c) 2025 Amarok Team <amarok@kde.org>                                 *
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

#include "SqliteStorageFactory.h"
#include "SqliteStorage.h"

#include <core/support/Amarok.h>

#include <QDir>
#include <QStandardPaths>


SqliteStorageFactory::SqliteStorageFactory()
    : StorageFactory()
{
}

SqliteStorageFactory::~SqliteStorageFactory()
{
}

void
SqliteStorageFactory::init()
{
    if( m_initialized )
        return;

    m_initialized = true;

    // DatabaseBackend: 0 = Embedded MySQL, 1 = External MySQL, 2 = SQLite
    const int backend = Amarok::config( QStringLiteral("MySQL") ).readEntry( "DatabaseBackend", 0 );
    if( backend != 2 )
        return;

    QString dbPath = Amarok::config( QStringLiteral("SQLite") ).readEntry( "DatabasePath", QString() );

    if( dbPath.isEmpty() )
    {
        QString dataDir = QStandardPaths::writableLocation( QStandardPaths::AppDataLocation );
        QDir().mkpath( dataDir );
        dbPath = dataDir + QStringLiteral("/amarok.db");
    }

    SqliteStorage *storage = new SqliteStorage();
    bool initResult = storage->init( dbPath );

    if( !storage->getLastErrors().isEmpty() )
        Q_EMIT newError( storage->getLastErrors() );
    storage->clearLastErrors();

    if( initResult )
        Q_EMIT newStorage( QSharedPointer<SqlStorage>( storage ) );
    else
        delete storage;
}
