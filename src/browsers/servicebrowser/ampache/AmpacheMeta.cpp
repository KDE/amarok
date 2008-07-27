/***************************************************************************
 *   Copyright (c) 2007  Casey Link <unnamedrambler@gmail.com>             *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "AmpacheMeta.h"
#include "Debug.h"


using namespace Meta;

//// AmpacheAlbum ////

AmpacheAlbum::AmpacheAlbum( const QString &name )
    : ServiceAlbumWithCover( name )
{
}

AmpacheAlbum::AmpacheAlbum(const QStringList & resultRow)
    : ServiceAlbumWithCover( resultRow )
{
}

AmpacheAlbum::~ AmpacheAlbum()
{
}

void AmpacheAlbum::setCoverUrl( const QString &coverURL )
{
    m_coverURL = coverURL;
}

QString AmpacheAlbum::coverUrl( ) const
{
    return m_coverURL;
}

QList< PopupDropperAction * > Meta::AmpacheTrack::currentTrackActions()
{
    QList< PopupDropperAction * > actions;

    if ( !m_showInServiceAction )
    {
        m_showInServiceAction = new ShowInServiceAction( m_service, this );
    }

    actions.append( m_showInServiceAction );
    return actions;

}

