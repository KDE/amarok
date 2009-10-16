/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

#ifndef PLAYLISTVIEWURLGENERATOR_H
#define PLAYLISTVIEWURLGENERATOR_H

#include "amarok_export.h"
#include "AmarokUrlGenerator.h"
#include "AmarokUrl.h"

namespace Playlist
{

class AMAROK_EXPORT ViewUrlGenerator : public AmarokUrlGenerator
{
public:

    static ViewUrlGenerator * instance();

    QString description();
    AmarokUrl createUrl();

private:

    ViewUrlGenerator();
    virtual ~ViewUrlGenerator();
    
    static ViewUrlGenerator * s_instance;
};

}

#endif  //PLAYLISTVIEWURLGENERATOR_H
