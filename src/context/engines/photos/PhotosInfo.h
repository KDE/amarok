/****************************************************************************************
 * Copyright (c) 2009 Simon Esneault <simon.esneault@gmail.com>                         *
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

#ifndef AMAROK_PHOTOS_INFO
#define AMAROK_PHOTOS_INFO

#include "context/DataEngine.h"
#include "Debug.h"

//!  Struct PhotosInfo, contain all the info vor a photos
class PhotosInfo {

public:

    PhotosInfo()
    : photo( 0 )
    {}

    ~PhotosInfo()
    {
        delete photo;
    }
    
    QString title;      // Name of the phtos
    QString urlphoto;   // url of the photos, for the download
    QString urlpage;    // Url for the browser ( http://www.flickr.com/photos/wanderlustg/322285063/ )
    QPixmap *photo;     // Image data
};

#endif
