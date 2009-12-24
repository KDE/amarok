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

#ifndef SQLCOLLECTIONFACTORY_H
#define SQLCOLLECTIONFACTORY_H

#include "amarok_sqlcollection_export.h"

#include <QString>

class SqlCollection;

class AMAROK_SQLCOLLECTION_EXPORT SqlCollectionFactory
{
public:
    SqlCollectionFactory();

    SqlCollection* createSqlCollection( const QString &id, const QString &prettyName ) const;
};

#endif // SQLCOLLECTIONFACTORY_H
