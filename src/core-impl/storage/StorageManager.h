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

#ifndef AMAROK_STORAGEMANAGER_H
#define AMAROK_STORAGEMANAGER_H

#include "amarok_export.h"

#include <QObject>
#include <QList>
#include <QSharedPointer>
#include <QStringList>

namespace Plugins {
    class PluginFactory;
}

class SqlStorage;

/** Class managing the Amarok SqlStorage
 *
 *  This singleton class is the main responsible for providing everybody
 *  with the current SqlStorage.
 */
class AMAROK_EXPORT StorageManager : public QObject
{
    Q_OBJECT

    public:

        /** Get THE instance of the storage manager.
         *
         * This function will return the storage manager
         * that returns the sql storage to be used for Amarok.
         *
         * In addition to the SqlCollection a lot of other components
         * use a sql storage to persist data.
         *
         */
        static StorageManager *instance();

        /** Destroys the instance of the StorageManager.
         */
        static void destroy();

        /**
            retrieve an interface which allows client-code to store/load data in a relational database.
            Note: code using this method does NOT take ownership of the pointer, but may cache the pointer
            Note2: You should never modify the database unless you really really know what you do.
                   Using the SqlMeta (e.g. SqlRegistry or SqlTrack) is much better.
            @return Returns a pointer to the amarok wide SqlStorage or
                    to an internal dummy SqlStorage if that cannot be found.
                    It never returns a null pointer.
        */
        QSharedPointer<SqlStorage> sqlStorage() const;

        /**
         * Set the list of current factories
         *
         * For every factory that is a CollectionFactory uses it to create new
         * collections and register with this manager.
         */
        void setFactories( const QList<Plugins::PluginFactory*> &factories );

        /** Returns a list of the last sql errors.
          The list might not include every one error if the number
          is beyond a sensible limit.
          */
        QStringList getLastErrors() const;

        /** Clears the list of the last errors. */
        void clearLastErrors();

    private Q_SLOTS:

        /** Will be called whenever a factory emits a newStorage signal.
         *
         *  The first factory to emit this signal will get it's storage
         *  registered as "the" storage.
         *
         *  StorageManager will take ownership of the pointer and free it
         *  after all other plugins are done.
         */
        void slotNewStorage( QSharedPointer<SqlStorage> newStorage );

        /** Will be called whenever a factory emits a newError signal.
         *
         *  The factories will not emit the newStorage signal in case
         *  of initialization problems.
         *  In order to report their issues they will instead emit
         *  newError with the list of errors.
         */
        void slotNewError( QStringList errorMessageList );

    private:
        static StorageManager* s_instance;
        StorageManager();
        ~StorageManager();

        void init();


        Q_DISABLE_COPY( StorageManager )

        struct Private;
        Private * const d;
};

#endif /* AMAROK_STORAGEMANAGER_H */
