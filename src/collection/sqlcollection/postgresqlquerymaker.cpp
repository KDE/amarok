/*
 *  Copyright (c) 2007 Maximilian Kossick <maximilian.kossick@googlemail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "postgresqlquerymaker.h"

PostgreSqlQueryMaker::PostgreSqlQueryMaker( PostgreSqlCollection *collection )
    : SqlQueryBuilder( collection )
{
    //nothing to do
}

PostgreSqlQueryMaker::~PostgreSqlQueryMaker()
{
    //nothing to do
}

QString
PostgreSqlQueryMaker::likeCondition( const QString &text, bool anyBegin, bool anyEnd ) const
{
    QString escaped = text;
    escaped.replace( '/', "//" ).replace( '%', "/%" ).replace( '_', "/_" );
    escaped = escape( escaped );

    QString ret = " ILIKE "; //case-insensitive according to locale

    ret += '\'';
    if ( anyBegin )
            ret += '%';
    ret += escaped;
    if ( anyEnd )
            ret += '%';
    ret += '\'';

    //Use / as the escape character
    ret += " ESCAPE '/' ";

    return ret;
}
