//Copyright (C) 2005 Max Howell <max.howell@methylblue.com>
//Licensed as described in the COPYING accompanying this distribution
//

#ifndef CRASH_H
#define CRASH_H

#include <kcrash.h> //for main.cpp

namespace amaroK
{
    /**
     * @author Max Howell
     * @short The amaroK crash-handler
     *
     * I'm not entirely sure why this had to be inside a class, but it
     * wouldn't work otherwise *shrug*
     */
    class Crash
    {
    public:
        static void crashHandler( int signal );
    };
}

#endif
