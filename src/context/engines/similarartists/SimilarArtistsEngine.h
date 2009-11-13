/****************************************************************************************
* Copyright (c) 2009 Nathan Sala <sala.nathan@gmail.com>                               *
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

#ifndef SIMILARARTISTSENGINE_H
#define SIMILARARTISTSENGINE_H

#include <context/ContextObserver.h>
#include <meta/Meta.h>


class SimilarArtistsEngine : public ContextObserver, public Meta::Observer
{
public:
    /**
    * Fetches the similar artists for an artist thanks to the LastFm WebService
    * @param artist_name the name of the artist
    * @return a map with the names of the artists with their match rate
    */
    QMap<int, QString> similarArtists(const QString &artist_name);
};

#endif // SIMILARARTISTSENGINE_H
