
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
 *        Debug::Block myBlock( __PRETTY_FUNCTION__ );
 *
 *        debug() << "output1" << endl;
 *        debug() << "output2" << endl;
 *     }
 *
 * Will output:
 *
 * app: BEGIN: void function()
 * app:   [Blah] output1
 * app:   [Blah] output2
 * app: END: void function(): Took 0.1s
 *
 * @see Block
 * @see CrashHelper
 * @see ListStream
 */

namespace Debug
{
    /// this is defined in app.cpp
    extern QCString indent;

    #ifdef NDEBUG
        static inline kndbgstream debug()   { return kndbgstream(); }
        static inline kndbgstream warning() { return kndbgstream(); }
        static inline kndbgstream error()   { return kndbgstream(); }
        static inline kndbgstream fatal()   { return kndbgstream(); }

        typedef kndbgstream DebugStream;
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

        typedef kdbgstream DebugStream;

        #undef AMK_PREFIX
    #endif

    typedef kndbgstream NoDebugStream;
}

using Debug::debug;
using Debug::warning;
using Debug::error;
using Debug::fatal;
using Debug::DebugStream;

/// Standard function announcer
#define DEBUG_FUNC_INFO kdDebug() << Debug::indent << k_funcinfo << endl;

/// Convenience macro for making a standard Debug::Block
#define DEBUG_BLOCK Debug::Block uniquelyNamedStackAllocatedStandardBlock( __FUNCTION__ );

#define DEBUG_INDENT Debug::indent += "  ";
#define DEBUG_UNINDENT Debug::indent.truncate( Debug::indent.length() - 2 );

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
     *         Debug::Block myBlock( "section" );
     *
     *         debug() << "output1" << endl;
     *         debug() << "output2" << endl;
     *     }
     *
     * Will output:
     *
     *     app: BEGIN: section
     *     app:  [prefix] output1
     *     app:  [prefix] output2
     *     app: END: section - Took 0.1s
     *
     */

    class Block
    {
        std::clock_t m_start;
        const char  *m_label;

    public:
        Block( const char *label )
                : m_start( std::clock() )
                , m_label( label )
        {
            kdDebug() << indent << "BEGIN: " << label << "\n";
            DEBUG_INDENT
        }

       ~Block()
        {
            std::clock_t finish = std::clock();
            const double duration = (double) (finish - m_start) / CLOCKS_PER_SEC;

            DEBUG_UNINDENT
            kdDebug() << indent << "END: " << m_label << " - Took " << duration << "s\n";
        }
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
     *
     */

    class CrashHelper : public Block
    {
        int m_counter;

    public:
        CrashHelper( const char *label = 0 ) : Block( label ), m_counter( 0 ) {}

        inline void stamp()
        {
            debug() <<  ": " << ++m_counter << endl;
        }
    };
}


#include <qvariant.h>

namespace Debug
{
    /**
     * @class Debug::ListStream
     * @short You can pass anything to this and it will output it as a list
     *
     * You can't allocate one, instead use debug::list(). Usage:
     *
     *     {
     *         Debug::list( "My list of stuff" )
     *                 << anInt        //5
     *                 << aString      //"moo"
     *                 << aQStringList //{"baa","neigh","woof"}
     *                 << aDouble;     //3.141
     *     }
     *
     * Will output:
     *
     *     app: BEGIN: My list of stuff
     *     app:   5
     *     app:   moo
     *     app:   { baa, neigh, woof }
     *     app:   3.141
     *     app: END: My list of stuff - Took 0.3s
     *
     * Note, don't end the sequence with endl, I couldn't get
     * that to parse. Sorry.
     */

    class ListStream : private Debug::Block
    {
        friend ListStream list();

        ListStream( const char *header = "List" ) : Block( header ), d( 0, 0 ) {}

        DebugStream d;

        public:
            ~ListStream() {}

            inline ListStream &operator<<( const QVariant &variant )
            {
                d << Debug::indent;
                d << variant.toString();
                d << endl;

                return *this;
            }
    };

    inline ListStream list() { return ListStream(); }
}

#endif
