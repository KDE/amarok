/****************************************************************************************
 * Copyright (c) 2009 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
#ifndef AMAROKURLGENERATOR_H
#define AMAROKURLGENERATOR_H

#include "AmarokUrl.h"

#include <KIcon>

class AmarokUrlGenerator
{

public:
    virtual ~AmarokUrlGenerator() {};

    /**
       Get the user visible description of what the createUrl() function will actualy bookmarks.
    */
    virtual QString description() = 0;

    /**
       Get the the icon for the type of bookmarks created.
    */
    virtual KIcon icon() = 0;

    /**
       Cretate the default url for this generator.
    */
    virtual AmarokUrl createUrl() = 0;
};

#endif // AMAROKURLGENERATOR_H
