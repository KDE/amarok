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

#include "QueryMaker.h"

QueryMaker::QueryMaker() : QObject()
{
}

QueryMaker::~QueryMaker()
{
}

int
QueryMaker::resultCount() const
{
    return 1;
}

QueryMaker*
QueryMaker::setAlbumQueryMode( AlbumQueryMode mode )
{
    Q_UNUSED( mode )
    return this;
}

int QueryMaker::validFilterMask()
{
    return AllFilters;
}


Meta::TrackList
QueryMaker::tracks( const QString &id ) const
{
    Meta::DataList dataList = data( id );

    if( !dataList.isEmpty() && Meta::TrackPtr::dynamicCast( dataList.first() ) )
    {
        Meta::TrackList list;
        foreach( Meta::DataPtr p, dataList )
        {
            list << Meta::TrackPtr::dynamicCast( p );
            if( !list.last() ) list.removeLast();
        }
        return list;
    }
    else
        return Meta::TrackList();
}



Meta::AlbumList
QueryMaker::albums( const QString &id ) const
{
    Meta::DataList dataList = data( id );

    if( !dataList.isEmpty() && Meta::AlbumPtr::dynamicCast( dataList.first() ) )
    {
        Meta::AlbumList list;
        foreach( Meta::DataPtr p, dataList )
        {
            list << Meta::AlbumPtr::dynamicCast( p );
            if( !list.last() ) list.removeLast();
        }
        return list;
    }
    else
        return Meta::AlbumList();
}


Meta::ArtistList
QueryMaker::artists( const QString &id ) const
{
    Meta::DataList dataList = data( id );

    if( !dataList.isEmpty() && Meta::ArtistPtr::dynamicCast( dataList.first() ) )
    {
        Meta::ArtistList list;
        foreach( Meta::DataPtr p, dataList )
        {
            list << Meta::ArtistPtr::dynamicCast( p );
            if( !list.last() ) list.removeLast();
        }
        return list;
    }
    else
        return Meta::ArtistList();
}


Meta::GenreList
QueryMaker::genres( const QString &id ) const
{
    Meta::DataList dataList = data( id );

    if( !dataList.isEmpty() && Meta::GenrePtr::dynamicCast( dataList.first() ) )
    {
        Meta::GenreList list;
        foreach( Meta::DataPtr p, dataList )
        {
            list << Meta::GenrePtr::dynamicCast( p );
            if( !list.last() ) list.removeLast();
        }
        return list;
    }
    else
        return Meta::GenreList();
}


Meta::ComposerList
QueryMaker::composers( const QString &id ) const
{
    Meta::DataList dataList = data( id );

    if( !dataList.isEmpty() && Meta::ComposerPtr::dynamicCast( dataList.first() ) )
    {
        Meta::ComposerList list;
        foreach( Meta::DataPtr p, dataList )
        {
            list << Meta::ComposerPtr::dynamicCast( p );
            if( !list.last() ) list.removeLast();
        }
        return list;
    }
    else
        return Meta::ComposerList();
}


Meta::YearList
QueryMaker::years( const QString &id ) const
{
    Meta::DataList dataList = data( id );

    if( !dataList.isEmpty() && Meta::YearPtr::dynamicCast( dataList.first() ) )
    {
        Meta::YearList list;
        foreach( Meta::DataPtr p, dataList )
        {
            list << Meta::YearPtr::dynamicCast( p );
            if( !list.last() ) list.removeLast();
        }
        return list;
    }
    else
        return Meta::YearList();
}



#include "QueryMaker.moc"

