/****************************************************************************************
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "ArtworkCapability.h"

Handler::ArtworkCapability::~ArtworkCapability()
{
    // nothing to do here
}

void
Handler::ArtworkCapability::setCoverPath( Meta::MediaDeviceAlbumPtr album, const QString &path )
{
    const QPixmap pix( path );
    if( !pix.isNull() )
        setCover( album, pix );
}

#include "ArtworkCapability.moc"
