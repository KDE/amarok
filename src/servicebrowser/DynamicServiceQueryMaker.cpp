/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *             (c) 2007 Adam Pigg <adam@piggz.co.uk>                       *
 *                                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.         *
 ***************************************************************************/

#include "DynamicServiceQueryMaker.h"

#include "debug.h"

using namespace Meta;



DynamicServiceQueryMaker::DynamicServiceQueryMaker( )
 : QueryMaker()
{
}

QueryMaker * DynamicServiceQueryMaker::addReturnValue(qint64 value)
{
    Q_UNUSED( value );
    return this;
}

QueryMaker * DynamicServiceQueryMaker::orderBy(qint64 value, bool descending)
{
    Q_UNUSED( value );
    return this;
}

QueryMaker * DynamicServiceQueryMaker::includeCollection(const QString & collectionId)
{
    Q_UNUSED( collectionId );
    return this;
}

QueryMaker * DynamicServiceQueryMaker::excludeCollection(const QString & collectionId)
{
    Q_UNUSED( collectionId );
    return this;
}

QueryMaker * DynamicServiceQueryMaker::addMatch(const Meta::TrackPtr & track)
{
    Q_UNUSED( track );
    return this;
}

QueryMaker * DynamicServiceQueryMaker::addMatch(const Meta::ArtistPtr & artist)
{
    Q_UNUSED( artist );
    return this;
}

QueryMaker * DynamicServiceQueryMaker::addMatch(const Meta::AlbumPtr & album)
{
    Q_UNUSED( album );
    return this;
}

QueryMaker * DynamicServiceQueryMaker::addMatch(const Meta::GenrePtr & genre)
{
    Q_UNUSED( genre );
    return this;
}

QueryMaker * DynamicServiceQueryMaker::addMatch(const Meta::ComposerPtr & composer)
{
    Q_UNUSED( composer );
    return this;
}

QueryMaker * DynamicServiceQueryMaker::addMatch(const Meta::YearPtr & year)
{
    Q_UNUSED( year );
    return this;
}

QueryMaker * DynamicServiceQueryMaker::addMatch(const Meta::DataPtr & data)
{
    Q_UNUSED( data );
    return this;
}

QueryMaker * DynamicServiceQueryMaker::addFilter(qint64 value, const QString & filter, bool matchBegin, bool matchEnd)
{
    Q_UNUSED( value );
    Q_UNUSED( filter );
    Q_UNUSED( matchBegin );
    Q_UNUSED( matchEnd );
    return this;
}

QueryMaker * DynamicServiceQueryMaker::excludeFilter(qint64 value, const QString & filter, bool matchBegin, bool matchEnd)
{
    Q_UNUSED( value );
    Q_UNUSED( filter );
    Q_UNUSED( matchBegin );
    Q_UNUSED( matchEnd );
    return this;
}

QueryMaker * DynamicServiceQueryMaker::limitMaxResultSize(int size)
{
    Q_UNUSED( size );
    return this;
}


#include "DynamicServiceQueryMaker.moc"






