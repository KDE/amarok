/***************************************************************************
 *   Copyright (C) 2005 Max Howell <max.howell@methylblue.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_CRASH_H
#define AMAROK_CRASH_H

#include <KCrash>        //for main.cpp
#include <amarok_export.h>

namespace Amarok
{
    /**
     * @author Max Howell
     * @short The Amarok crash-handler
     *
     * I'm not entirely sure why this had to be inside a class, but it
     * wouldn't work otherwise *shrug*
     */
    class AMAROK_EXPORT Crash
    {
    public:
        static void crashHandler( int signal );
    };
}

#endif //AMAROK_CRASH_H

