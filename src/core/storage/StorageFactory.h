
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

#ifndef AMAROK_STORAGE_FACTORY_H
#define AMAROK_STORAGE_FACTORY_H

#include "core/amarokcore_export.h"
#include "core/support/PluginFactory.h"

#include <QObject>
#include <QSharedPointer>

class SqlStorage;

/** A plugin that provides a Storage object.
 *
 *  The storage plugin is the only part of Amarok that actually needs to be
 *  a plugin since we are linking two libraries (MySqlClient and MySqlEmbedded)
 *  that have the same symbols.
 *
 */
class AMAROK_CORE_EXPORT StorageFactory : public Plugins::PluginFactory
{
    Q_OBJECT

public:
    StorageFactory();
    virtual ~StorageFactory();

Q_SIGNALS:
    /** Emitted whenever the factory produces a new storage.
     *
     */
    void newStorage( QSharedPointer<SqlStorage> newStorage );

    /**
     *  The factories will not Q_EMIT the newStorage signal in case
     *  of initialization problems.
     *  In order to report their issues they will instead Q_EMIT
     *  newError with the list of errors.
     */
    void newError( QStringList errorMessageList );
};

#endif /* AMAROK_STORAGE_FACTORY_H */
