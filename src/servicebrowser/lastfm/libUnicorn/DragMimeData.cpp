/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Last.fm Ltd <client@last.fm>                                       *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "DragMimeData.h"

#include <QString>

Track
DragMimeData::track() const
{
    Track track;
    track.setArtist( QString::fromUtf8( data( "item/artist" ) ) );
    track.setTitle( QString::fromUtf8( data( "item/track" ) ) );
    track.setAlbum( QString::fromUtf8( data( "item/album" ) ) );
    return track;
}

QString
DragMimeData::tag() const
{
    return QString::fromUtf8( data( "item/tag" ) );
}

QString
DragMimeData::username() const
{
    return QString::fromUtf8( data( "item/user" ) );
}

Station
DragMimeData::station() const
{
    Station station;
    station.setUrl( QString::fromUtf8( data( "item/station" ) ) );
    return station;
}

UnicornEnums::ItemType
DragMimeData::itemType() const
{
    if (hasFormat("item/type"))
        return (UnicornEnums::ItemType) QString::fromUtf8(data("item/type")).toInt();

    if (hasFormat("item/track"))
        return UnicornEnums::ItemTrack;        
    if (hasFormat("item/album"))
        return UnicornEnums::ItemAlbum;
    if (hasFormat("item/artist")) //leave last as album and track have this data too
        return UnicornEnums::ItemArtist;
        
    return UnicornEnums::ItemUnknown;
}

QString
DragMimeData::toString() const
{
    switch ((int)itemType())
    {
        case UnicornEnums::ItemTrack:  return track().toString();
        case UnicornEnums::ItemAlbum:  return QString::fromUtf8( data( "item/album" ) );
        case UnicornEnums::ItemArtist: return QString::fromUtf8( data( "item/artist" ) ); //leave last as album and track have this data too
    }
    
    return QString();
}
