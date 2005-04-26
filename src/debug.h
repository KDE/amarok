// Author:    Max Howell <max.howell@methylblue.com>, (C) 2003-5
// Copyright: See COPYING file that comes with this distribution
//

#ifndef AMAROK_DEBUG_H
#define AMAROK_DEBUG_H

#include <kdebug.h>
#include <qcstring.h>
#include <qobject.h>
#include <sys/time.h>

class QApplication;
extern QApplication *qApp; ///@see Debug::Indent


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
    // we can't use a statically instantiated QCString for the indent, because
    // static namespaces are unique to each dlopened library. So we piggy back
    // the QCString on the KApplication instance

    #define qApp reinterpret_cast<QObject*>(qApp)
    class Indent : QObject
    {
        friend QCString &indent();
        Indent() : QObject( qApp, "DEBUG_indent" ) {}
        QCString m_string;
    };

    inline QCString &indent()
    {
        QObject *o = qApp ? qApp->child( "DEBUG_indent" ) : 0;
        return (o ? (Indent*)o : new Indent)->m_string;
    }
    #undef qApp


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

        static inline kdbgstream debug()   { return kdbgstream( indent(), 0, KDEBUG_INFO  ) << AMK_PREFIX; }
        static inline kdbgstream warning() { return kdbgstream( indent(), 0, KDEBUG_WARN  ) << AMK_PREFIX << "[WARNING!] "; }
        static inline kdbgstream error()   { return kdbgstream( indent(), 0, KDEBUG_ERROR ) << AMK_PREFIX << "[ERROR!] "; }
        static inline kdbgstream fatal()   { return kdbgstream( indent(), 0, KDEBUG_FATAL ) << AMK_PREFIX; }

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
#define DEBUG_FUNC_INFO kdDebug() << Debug::indent() << k_funcinfo << endl;

/// Announce a line
#define DEBUG_LINE_INFO kdDebug() << Debug::indent() << k_funcinfo << "Line: " << __LINE__ << endl;

/// Convenience macro for making a standard Debug::Block
#define DEBUG_BLOCK Debug::Block uniquelyNamedStackAllocatedStandardBlock( __PRETTY_FUNCTION__ );

#define DEBUG_INDENT Debug::indent() += "  ";
#define DEBUG_UNINDENT Debug::indent().truncate( Debug::indent().length() - 2 );

/// Use this to remind yourself to finish the implementation of a function
#define AMAROK_NOTIMPLEMENTED warning() << "NOT-IMPLEMENTED: " << __PRETTY_FUNCTION__ << endl;

/// Use this to alert other developers to stop using a function
#define AMAROK_DEPRECATED warning() << "DEPRECATED: " << __PRETTY_FUNCTION__ << endl;


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
        timeval     m_start;
        const char *m_label;

    public:
        Block( const char *label )
                : m_label( label )
        {
            gettimeofday( &m_start, 0 );

            kdDebug() << indent() << "BEGIN: " << label << "\n";
            DEBUG_INDENT
        }

        ~Block()
        {
            timeval end;
            gettimeofday( &end, 0 );

            end.tv_sec -= m_start.tv_sec;
            if( end.tv_usec < m_start.tv_usec) {
                // Manually carry a one from the seconds field.
                end.tv_usec += 1000000;
                end.tv_sec--;
            }
            end.tv_usec -= m_start.tv_usec;

            double duration = double(end.tv_sec) + (double(end.tv_usec) / 1000000.0);

            DEBUG_UNINDENT
            kdDebug() << indent() << "END__: " << m_label
                      << " - Took " << QString::number( duration, 'g', 2 ) << "s\n";
        }
    };


    /**
     * @name Debug::stamp()
     * @short To facilitate crash/freeze bugs, by making it easy to mark code that has been processed
     *
     * Usage:
     *
     *     {
     *         Debug::stamp();
     *         function1();
     *         Debug::stamp();
     *         function2();
     *         Debug::stamp();
     *     }
     *
     * Will output (assuming the crash occurs in function2()
     *
     *     app: Stamp: 1
     *     app: Stamp: 2
     *
     */

    inline void stamp()
    {
        static int n = 0;
        debug() << "| Stamp: " << ++n << endl;
    }
}


#include <qvariant.h>

namespace Debug
{
    /**
     * @class Debug::List
     * @short You can pass anything to this and it will output it as a list
     *
     *     debug() << (Debug::List() << anInt << aString << aQStringList << aDouble) << endl;
     */

    typedef QValueList<QVariant> List;
}

#endif
