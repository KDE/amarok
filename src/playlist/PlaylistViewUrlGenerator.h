/****************************************************************************************
 * Copyright (c) 2009 Téo Mrnjavac <teo@kde.org>                                        *
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
#include "amarokurls/AmarokUrl.h"
#include "amarokurls/AmarokUrlGenerator.h"

namespace Playlist
{

class AMAROK_EXPORT ViewUrlGenerator : public AmarokUrlGenerator
{
public:

    static ViewUrlGenerator * instance();

    QString description() override;
    QIcon icon() override;
    AmarokUrl createUrl() override;

private:

    ViewUrlGenerator();
    ~ViewUrlGenerator() override;
    
    static ViewUrlGenerator * s_instance;
};

}

#endif  //PLAYLISTVIEWURLGENERATOR_H
