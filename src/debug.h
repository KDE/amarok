
#ifndef AMAROK_DEBUG_H
#define AMAROK_DEBUG_H

#include <kdebug.h>

/**
 * Main reason for these is so you can define a DEBUG_PREFIX
 * that appears before your output. Just do:
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
    /// this is used by DEBUG_BEGIN and DEBUG_END (defined in app.cpp)
    extern QCString __indent;

    #ifdef NDEBUG
        static inline kndbgstream debug()   { return kndbgstream(); }
        static inline kndbgstream warning() { return kndbgstream(); }
        static inline kndbgstream error()   { return kndbgstream(); }
        static inline kndbgstream fatal()   { return kndbgstream(); }
    #else
        #ifndef DEBUG_PREFIX
        #define AMK_PREFIX ""
        #else
        #define AMK_PREFIX "[" DEBUG_PREFIX "] "
        #endif

        //from kdebug.h
        enum DebugLevels {
            KDEBUG_INFO  = 0,
            KDEBUG_WARN  = 1,
            KDEBUG_ERROR = 2,
            KDEBUG_FATAL = 3
        };

        static inline kdbgstream debug()   { return kdbgstream( __indent, 0, KDEBUG_INFO  ) << AMK_PREFIX; }
        static inline kdbgstream warning() { return kdbgstream( __indent, 0, KDEBUG_WARN  ) << AMK_PREFIX; }
        static inline kdbgstream error()   { return kdbgstream( __indent, 0, KDEBUG_ERROR ) << AMK_PREFIX; }
        static inline kdbgstream fatal()   { return kdbgstream( __indent, 0, KDEBUG_FATAL ) << AMK_PREFIX; }

        #undef AMK_PREFIX
    #endif
}

using namespace Debug;

/// Standard function announcer
#define DEBUG_FUNC_INFO kdDebug() << Debug::__indent << k_funcinfo << endl;

#define DEBUG_INDENT Debug::__indent += "  ";
#define DEBUG_UNDENT Debug::__indent.truncate( Debug::__indent.length() - 2 );

/// Use this to introduce a function
#define DEBUG_BEGIN kdDebug() << __indent << "BEGIN: " << __PRETTY_FUNCTION__ << endl; DEBUG_INDENT

/// Use this to extroduce a function
#define DEBUG_END   DEBUG_UNDENT kdDebug() << __indent << "END: " << __PRETTY_FUNCTION__ << endl;

/// Use this to remind yourself to finish the implementation of a function
#define AMAROK_NOTIMPLEMENTED kdWarning() << "NOT-IMPLEMENTED: " << __PRETTY_FUNCTION__ << endl;

/// Use this to alert other developers to stop using a function
#define AMAROK_DEPRECATED kdWarning() << "DEPRECATED: " << __PRETTY_FUNCTION__ << endl;

#endif
