/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
 * Copyright (c) 2009 Téo Mrnjavac <teo.mrnjavac@gmail.com>                             *
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

#ifndef PLAYLISTVIEWURLRUNNER_H
#define PLAYLISTVIEWURLRUNNER_H

#include "amarokurls/AmarokUrl.h"
#include "amarokurls/AmarokUrlRunnerBase.h"

#include <KIcon>

namespace Playlist
{

class ViewUrlRunner : public AmarokUrlRunnerBase
{
public:
    ViewUrlRunner();
    virtual ~ViewUrlRunner();

    virtual QString command() const;
    virtual KIcon icon() const;
    virtual bool run( AmarokUrl url );
};

}

#endif  //PLAYLISTVIEWURLRUNNER_H
