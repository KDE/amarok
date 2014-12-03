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

    virtual QString type() const { return QLatin1String("Empty"); }

    virtual QString escape( const QString &text) const { return text; }

    virtual QStringList query( const QString &) { return QStringList(); }
    virtual int insert( const QString &, const QString &) { return 0; }

    virtual QString boolTrue() const { return QString(); }
    virtual QString boolFalse() const { return QString(); }

    virtual QString idType() const { return QString(); };
    virtual QString textColumnType( int ) const { return QString(); }
    virtual QString exactTextColumnType( int ) const { return QString(); }

    virtual QString exactIndexableTextColumnType( int ) const { return QString(); }
    virtual QString longTextColumnType() const { return QString(); }
    virtual QString randomFunc() const { return QString(); }

    virtual QStringList getLastErrors() const { return QStringList(); }

    /** Clears the list of the last errors. */
    virtual void clearLastErrors() { }

};

static EmptySqlStorage emptyStorage;


struct StorageManager::Private
{
    SqlStorage* sqlDatabase;

    /** A list that collects errors from database plugins
     *
     *  StoragePlugin factories can report errors that
     *  prevent the storage from even being created.
     *
     *  This list collects them.
     */
    QStringList errorList;
};

StorageManager *StorageManager::s_instance = 0;


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
        s_instance = 0;
    }
}

StorageManager::StorageManager()
    : QObject()
    , d( new Private )
{
    DEBUG_BLOCK

    setObjectName( "StorageManager" );
    qRegisterMetaType<SqlStorage *>( "SqlStorage*" );
    d->sqlDatabase = &emptyStorage;
}

StorageManager::~StorageManager()
{
    DEBUG_BLOCK

    if( d->sqlDatabase != &emptyStorage )
        delete d->sqlDatabase;
    delete d;
}

SqlStorage*
StorageManager::sqlStorage() const
{
    return d->sqlDatabase;
}

void
StorageManager::init()
{
}


void
StorageManager::setFactories( const QList<Plugins::PluginFactory*> &factories )
{
    foreach( Plugins::PluginFactory* pFactory, factories )
    {
        StorageFactory *factory = qobject_cast<StorageFactory*>( pFactory );
        if( !factory )
            continue;

        connect( factory, SIGNAL(newStorage(SqlStorage*)),
                 this, SLOT(slotNewStorage(SqlStorage*)) );
        connect( factory, SIGNAL(newError(QStringList)),
                 this, SLOT(slotNewError(QStringList)) );
    }
}

QStringList
StorageManager::getLastErrors() const
{
    if( !d->errorList.isEmpty() )
        return d->errorList;
    if( d->sqlDatabase == &emptyStorage )
    {
        QStringList list;
        list << i18n( "The configured database plugin could be loaded." );
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
StorageManager::slotNewStorage( SqlStorage* newStorage )
{
    DEBUG_BLOCK

    if( !newStorage )
    {
        warning() << "Warning, newStorage in slotNewStorage is 0";
        return;
    }

    if( d->sqlDatabase && d->sqlDatabase != &emptyStorage )
    {
        warning() << "Warning, newStorage when we already have a storage";
        delete newStorage;
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


