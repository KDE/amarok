
#ifndef AMAROK_DEBUG_H
#define AMAROK_DEBUG_H

#include <kdebug.h>
#include <qcstring.h>

/**
 * @namespace Debug
 * @short kdebug with indentation functionality and convenience macros
 * @author Max Howell <max.howell@methylblue.com>
 *
 * Usage:
 *
 *     #define DEBUG_PREFIX "Blah"
 *     #include "debug.h"
 *
 *     void function()
 *     {
 *        DEBUG_BEGIN
 *        debug() << "output1" << endl;
 *        debug() << "output2" << endl;
 *        DEBUG_END
 *     }
 *
 * Will output:
 *
 * app: BEGIN: [void function()]
 * app:   [Blah] output1
 * app:   [Blah] output2
 * app: END: [void function()]
 *
 * @see DebugSection
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

        typedef kndbgstream debugstream;
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

        typedef kdbgstream debugstream;

        #undef AMK_PREFIX
    #endif
}

using namespace Debug;

/// Standard function announcer
#define DEBUG_FUNC_INFO kdDebug() << Debug::__indent << k_funcinfo << endl;

#define DEBUG_INDENT Debug::__indent += "  ";
#define DEBUG_UNINDENT Debug::__indent.truncate( Debug::__indent.length() - 2 );

/// Use this to introduce a function
#define DEBUG_BEGIN kdDebug() << __indent << "BEGIN: " << __PRETTY_FUNCTION__ << endl; DEBUG_INDENT

/// Use this to extroduce a function
#define DEBUG_END   DEBUG_UNINDENT kdDebug() << __indent << "END: " << __PRETTY_FUNCTION__ << endl;

/// Use this to remind yourself to finish the implementation of a function
#define AMAROK_NOTIMPLEMENTED kdWarning() << "NOT-IMPLEMENTED: " << __PRETTY_FUNCTION__ << endl;

/// Use this to alert other developers to stop using a function
#define AMAROK_DEPRECATED kdWarning() << "DEPRECATED: " << __PRETTY_FUNCTION__ << endl;

namespace Debug
{
    /**
     * @class DebugSection
     * @short Use this to label sections of your code
     *
     * Usage:
     *
     *     void function()
     *     {
     *         DebugSection s( "section" );
     *
     *         debug() << "output1" << endl;
     *         debug() << "output2" << endl;
     *     }
     *
     * Will output:
     *
     *     app: BEGIN: section
     *     app:  output1
     *     app:  output2
     *     app: END: section
     */

    class DebugSection
    {
    public:
        DebugSection( const char *label ) : m_label( label )
        {
            kdDebug() << __indent << "BEGIN: " << m_label << endl;
            DEBUG_INDENT
        }

        ~DebugSection()
        {
            DEBUG_UNINDENT
            kdDebug() << __indent << "END: " << m_label << endl;
        }

    private:
        const char *m_label;
    };
}

#endif
