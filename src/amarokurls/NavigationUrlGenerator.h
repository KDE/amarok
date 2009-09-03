/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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
 
#ifndef NAVIGATIONURLGENERATOR_H
#define NAVIGATIONURLGENERATOR_H

#include "amarok_export.h"
#include "Meta.h"

class AmarokUrl;

/**
A class used to generate navigation urls.

	@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com> 
*/
class AMAROK_EXPORT NavigationUrlGenerator{
public:
    NavigationUrlGenerator();
    ~NavigationUrlGenerator();

    AmarokUrl CreateAmarokUrl();

    AmarokUrl urlFromAlbum( Meta::AlbumPtr album );
    AmarokUrl urlFromArtist( Meta::ArtistPtr artist );

};

#endif
