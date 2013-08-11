/****************************************************************************************
 * Copyright (c) 2007 Casey Link <unnamedrambler@gmail.com>                             *
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

#include "AmpacheMeta.h"
#include "core/support/Debug.h"

using namespace Meta;

//// AmpacheAlbum ////

AmpacheAlbum::AmpacheAlbum( const QString &name )
    : ServiceAlbumWithCover( name )
{}

AmpacheAlbum::AmpacheAlbum(const QStringList & resultRow)
    : ServiceAlbumWithCover( resultRow )
{}

AmpacheAlbum::~ AmpacheAlbum()
{}

void AmpacheAlbum::setCoverUrl( const QString &coverURL )
{
    m_coverURL = coverURL;
}

QString AmpacheAlbum::coverUrl( ) const
{
    return m_coverURL;
}

void
AmpacheAlbum::addInfo( const AmpacheAlbum::AmpacheAlbumInfo &info )
{
    m_ampacheAlbums.insert( info.id, info );
}

AmpacheAlbum::AmpacheAlbumInfo
AmpacheAlbum::getInfo( int id ) const
{
    if( !m_ampacheAlbums.contains( id ) )
    {
        AmpacheAlbumInfo info;
        info.id = -1;
        info.discNumber = -1;
        info.year = -1;
        return info;
    }
    return m_ampacheAlbums.value( id );
}

QString
AmpacheTrack::notPlayableReason() const
{
    return networkNotPlayableReason();
}
