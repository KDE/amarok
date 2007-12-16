/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Erik Jalevik, Last.fm Ltd <erik@last.fm>                           *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "StationUrl.h"
#include <QUrl>

StationUrl::StationUrl( const QString& url )
        : QString( url )
{}


StationUrl::StationUrl( const QUrl& url )
        : QString( url.toString() )
{}

    
bool
StationUrl::isPlaylist()
{
    return startsWith( "lastfm://play/" )    ||
           startsWith( "lastfm://preview/" ) ||
           startsWith( "lastfm://track/" )   ||
           startsWith( "lastfm://playlist/" );
}

