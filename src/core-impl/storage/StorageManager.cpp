/****************************************************************************************
 * Copyright (c) 2014 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#define DEBUG_PREFIX "StorageManager"

#include "StorageManager.h"

#include <core/storage/SqlStorage.h>
#include <core/storage/StorageFactory.h>

#include <core/support/Amarok.h>
#include <core/support/Debug.h>

#include <KConfigGroup>
#include <KLocalizedString>

/** A SqlStorage that doesn't do anything.
 *
 *  An object of this type is used whenever we couldn't
 *  load a better SqlStorage.
 *
 *  The reason is that plugins don't have to check for
 *  a null pointer as SqlStorage every time.
 */
class EmptySqlStorage : public SqlStorage
{
public:
    EmptySqlStorage() {}
    virtual ~EmptySqlStorage() {}

    virtual int sqlDatabasePriority() const
    { return 10; }

    virtual QString type() const { return QStringLiteral("Empty"); }

    QString escape( const QString &text) const override { return text; }

    QStringList query( const QString &) override { return QStringList(); }
    int insert( const QString &, const QString &) override { return 0; }

    QString boolTrue() const override { return QString(); }
    QString boolFalse() const override { return QString(); }

    QString idType() const override { return QString(); }
    QString textColumnType( int ) const override { return QString(); }
    QString exactTextColumnType( int ) const override { return QString(); }

    QString exactIndexableTextColumnType( int ) const override { return QString(); }
    QString longTextColumnType() const override { return QString(); }
    QString randomFunc() const override { return QString(); }

    QStringList getLastErrors() const override { return QStringList(); }

    /** Clears the list of the last errors. */
    void clearLastErrors() override { }
};


struct StorageManager::Private
{
    QSharedPointer<SqlStorage> sqlDatabase;

    /** A list that collects errors from database plugins
     *
     *  StoragePlugin factories can report errors that
     *  prevent the storage from even being created.
     *
     *  This list collects them.
     */
    QStringList errorList;
};

StorageManager *StorageManager::s_instance = nullptr;


StorageManager *
StorageManager::instance()
{
    if( !s_instance ) {
        s_instance = new StorageManager();
        s_instance->init();
    }

    return s_instance;
}

void
StorageManager::destroy()
{
    if( s_instance ) {
        delete s_instance;
        s_instance = nullptr;
    }
}

StorageManager::StorageManager()
    : QObject()
    , d( new Private )
{
    DEBUG_BLOCK

    setObjectName( QStringLiteral("StorageManager") );
    qRegisterMetaType<SqlStorage *>( "SqlStorage*" );
    d->sqlDatabase = QSharedPointer<SqlStorage>( new EmptySqlStorage );
}

StorageManager::~StorageManager()
{
    DEBUG_BLOCK

    delete d;
}

QSharedPointer<SqlStorage>
StorageManager::sqlStorage() const
{
    return d->sqlDatabase;
}

void
StorageManager::init()
{
}

void
StorageManager::setFactories( const QList<QSharedPointer<Plugins::PluginFactory> > &factories )
{
    for( const auto &pFactory : factories )
    {
        auto factory = qobject_cast<StorageFactory*>( pFactory );
        if( !factory )
            continue;

        connect( factory.data(), &StorageFactory::newStorage,
                 this, &StorageManager::slotNewStorage );
        connect( factory.data(), &StorageFactory::newError,
                 this, &StorageManager::slotNewError );
    }
}

QStringList
StorageManager::getLastErrors() const
{
    if( !d->errorList.isEmpty() )
        return d->errorList;
    if( d->sqlDatabase.dynamicCast<EmptySqlStorage>() )
    {
        QStringList list;
        list << i18n( "The configured database plugin could not be loaded." );
        return list;
    }
    return d->errorList;
}

void
StorageManager::clearLastErrors()
{
    d->errorList.clear();
}

void
StorageManager::slotNewStorage( QSharedPointer<SqlStorage> newStorage )
{
    DEBUG_BLOCK

    if( !newStorage )
    {
        warning() << "Warning, newStorage in slotNewStorage is 0";
        return;
    }

    if( d->sqlDatabase && !d->sqlDatabase.dynamicCast<EmptySqlStorage>() )
    {
        warning() << "Warning, newStorage when we already have a storage";
        return; // once we have the database set we can't change it since
        // plugins might have already created their tables in the old database
        // or caching data from it.
    }

    d->sqlDatabase = newStorage;
}

void
StorageManager::slotNewError( QStringList errorMessageList )
{
    d->errorList << errorMessageList;
}


