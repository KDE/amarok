
#ifndef AMAROK_DEBUG_H
#define AMAROK_DEBUG_H

#include <kdebug.h>

/**
 * Main reason for these is so you can define a DEBUG_PREFIX
 * that appears before your outpur. Just do:
 *
 * #define DEBUG_PREFIX "Blah"
 * #include "debug.h"
 *
 * add you debug output will appear like so:
 *
 * amaroK: [Blah] output
 */

namespace Debug
{
    #ifdef NDEBUG
        static inline kndbgstream debug()   { return kndbgstream(); }
        static inline kndbgstream warning() { return kndbgstream(); }
        static inline kndbgstream error()   { return kndbgstream(); }
        static inline kndbgstream fatal()   { return kndbgstream(); }
    #else
        #ifndef DEBUG_PREFIX
        #define _DEBUG_PREFIX ""
        #else
        #define _DEBUG_PREFIX "[" DEBUG_PREFIX "] "
        #endif

        //from kdebug.h
        enum DebugLevels {
            KDEBUG_INFO  = 0,
            KDEBUG_WARN  = 1,
            KDEBUG_ERROR = 2,
            KDEBUG_FATAL = 3
        };

        static inline kdbgstream debug()   { return kdbgstream( _DEBUG_PREFIX, 0, KDEBUG_INFO ); }
        static inline kdbgstream warning() { return kdbgstream( _DEBUG_PREFIX, 0, KDEBUG_WARN ); }
        static inline kdbgstream error()   { return kdbgstream( _DEBUG_PREFIX, 0, KDEBUG_ERROR ); }
        static inline kdbgstream fatal()   { return kdbgstream( _DEBUG_PREFIX, 0, KDEBUG_FATAL ); }

        #undef _DEBUG_PREFIX
    #endif
}

using namespace Debug;

/// Standard function announcer
#define DEBUG_FUNC_INFO kdDebug() << k_funcinfo << endl;

/// Use these to introduce and extroduce functions
#define DEBUG_SECTION_BEGIN kdDebug() << ">> " << __PRETTY_FUNCTION__ << endl;
#define DEBUG_SECTION_END   kdDebug() << "<< " << __PRETTY_FUNCTION__ << endl;

#define AMAROK_NOTIMPLEMENTED kdDebug() << "NOT-IMPLEMENTED: " << __PRETTY_FUNCTION__ << endl;
#define AMAROK_DEPRECATED kdDebug() << "DEPRECATED: " << __PRETTY_FUNCTION__ << endl;

#endif
