
#ifndef AMAROK_DEBUG_H
#define AMAROK_DEBUG_H

#include <ctime>       //std::clock_t
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
 * @see Block
 * @see CrashHelper
 * @see Timer
 */

namespace Debug
{
    /// this is used by DEBUG_BEGIN and DEBUG_END (defined in app.cpp, enginebase.cpp)
    extern QCString indent;

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

        static inline kdbgstream debug()   { return kdbgstream( indent, 0, KDEBUG_INFO  ) << AMK_PREFIX; }
        static inline kdbgstream warning() { return kdbgstream( indent, 0, KDEBUG_WARN  ) << AMK_PREFIX; }
        static inline kdbgstream error()   { return kdbgstream( indent, 0, KDEBUG_ERROR ) << AMK_PREFIX; }
        static inline kdbgstream fatal()   { return kdbgstream( indent, 0, KDEBUG_FATAL ) << AMK_PREFIX; }

        typedef kdbgstream debugstream;

        #undef AMK_PREFIX
    #endif
}

using Debug::debug;
using Debug::warning;
using Debug::error;
using Debug::fatal;
using Debug::debugstream;

/// Standard function announcer
#define DEBUG_FUNC_INFO kdDebug() << Debug::indent << k_funcinfo << endl;

#define DEBUG_INDENT Debug::indent += "  ";
#define DEBUG_UNINDENT Debug::indent.truncate( Debug::indent.length() - 2 );

/// Use this to introduce a function
#define DEBUG_BEGIN kdDebug() << Debug::indent << "BEGIN: " << __PRETTY_FUNCTION__ << endl; DEBUG_INDENT

/// Use this to extroduce a function
#define DEBUG_END   DEBUG_UNINDENT kdDebug() << Debug::indent << "END: " << __PRETTY_FUNCTION__ << endl;

/// Use this to remind yourself to finish the implementation of a function
#define AMAROK_NOTIMPLEMENTED kdWarning() << "NOT-IMPLEMENTED: " << __PRETTY_FUNCTION__ << endl;

/// Use this to alert other developers to stop using a function
#define AMAROK_DEPRECATED kdWarning() << "DEPRECATED: " << __PRETTY_FUNCTION__ << endl;

namespace Debug
{
    /**
     * @class Debug::Block
     * @short Use this to label sections of your code
     *
     * Usage:
     *
     *     void function()
     *     {
     *         Debug::Block s( "section" );
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
     *
     * Generally this is the easiest way to highlight a block of code.
     * Also it is worth noting that it is the only way to highlight blocks
     * like these:
     *
     *     {
     *       DEBUG_BEGIN
     *       return someFunction();
     *       DEBUG_END
     *     }
     *
     * DEBUG_END is never processed.
     *
     */

    class Block
    {
    public:
        Block( const char *label ) : m_label( label )
        {
            kdDebug() << indent << "BEGIN: " << m_label << endl;
            DEBUG_INDENT
        }

        ~Block()
        {
            DEBUG_UNINDENT
            kdDebug() << indent << "END: " << m_label << endl;
        }

    protected:
        const char *m_label;
    };


    /**
     * @class Debug::CrashHelper
     * @short To facilitate crash/freeze bugs, by making it easy to mark code that has been processed
     *
     * Usage:
     *
     *     {
     *         Debug::CrashHelper d( "Crash test" );
     *
     *         d.stamp();
     *         function1();
     *         d.stamp();
     *         function2();
     *         d.stamp();
     *     }
     *
     * Will output (assuming the crash occurs in function2()
     *
     *     app: BEGIN: Crash Test
     *     app:   [section] 1
     *     app:   [section] 2
     *     app: END: Crash Test
     *
     */

    class CrashHelper : public Block
    {
        int m_counter;
        const char *m_label;

    public:
        CrashHelper( const char *label = 0 ) : Block( label ), m_counter( 0 )
        {}

        inline void stamp()
        {
            debug() <<  ": " << ++m_counter << endl;
        }
    };


    /**
     * @class Debug::Timer
     * @short When destroyed it will output the time in seconds since it was created
     */

    class Timer
    {
        std::clock_t m_start;

    public:
        Timer( const char *label );
       ~Timer();
    };
}

#endif
