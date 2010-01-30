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

#ifndef DEFAULTSQLQUERYMAKERFACTORY_H
#define DEFAULTSQLQUERYMAKERFACTORY_H

#include "SqlQueryMaker.h"

class DefaultSqlQueryMakerFactory : public SqlQueryMakerFactory
{
public:
    DefaultSqlQueryMakerFactory( SqlCollection *collection )
        : SqlQueryMakerFactory()
        , m_collection( collection ) {}

    SqlQueryMaker *createQueryMaker() const
    {
        Q_ASSERT( m_collection );
        return new SqlQueryMaker( m_collection );
    }

private:
    SqlCollection *m_collection;
};

#endif // DEFAULTSQLQUERYMAKERFACTORY_H
