// Author:    Max Howell <max.howell@methylblue.com>, (C) 2003-5
// Copyright: See COPYING file that comes with this distribution
//

#ifndef AMAROK_DEBUG_H
#define AMAROK_DEBUG_H

#include <KConfig>
#include <kdebug.h>
#include <QApplication>
#include <QMutex>
#include <QObject>

#include <iostream>
#include <sys/time.h>

#include "amarok_export.h"

#ifdef _WIN32
#define __PRETTY_FUNCTION__ __FUNCTION__
#endif

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
    extern AMAROK_EXPORT QMutex mutex; // defined in app.cpp

    // we can't use a statically instantiated QCString for the indent, because
    // static namespaces are unique to each dlopened library. So we piggy back
    // the QCString on the KApplication instance

    #define qOApp reinterpret_cast<QObject*>(qApp)
    class Indent : QObject
    {
        friend QString &modifieableIndent();
        Indent() : QObject( qOApp ) { setObjectName( "DEBUG_indent" ); }
        QString m_string;
    };

    inline QString &modifieableIndent()
    {
        QObject* o = qOApp ? qOApp->findChild<QObject*>( "DEBUG_indent" ) : 0;
        QString &ret = (o ? static_cast<Indent*>( o ) : new Indent)->m_string;
        return ret;
    }

    inline QString indent()
    {
        return modifieableIndent();
    }

    inline bool debugEnabled()
    {
        KConfigGroup config = KGlobal::config()->group( "General" );
        const bool debug = config.readEntry( "Debug Enabled", false );
        
        return debug; 
    }

    inline kdbgstream dbgstream()
    {
        static QString nodebug;
        return debugEnabled() ? kdbgstream( QtDebugMsg ) : kdbgstream( &nodebug );
    }
 
    #undef qApp

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

        
    static inline kdbgstream debug()   { mutex.lock(); QString ind = indent(); mutex.unlock(); return dbgstream() << qPrintable( ind + AMK_PREFIX ); }
    static inline kdbgstream warning() { mutex.lock(); QString ind = indent(); mutex.unlock(); return dbgstream() << qPrintable( ind + AMK_PREFIX + "[WARNING!] " ); }
    static inline kdbgstream error()   { mutex.lock(); QString ind = indent(); mutex.unlock(); return dbgstream() << qPrintable( ind + AMK_PREFIX + "[ERROR!] " ); }
    static inline kdbgstream fatal()   { mutex.lock(); QString ind = indent(); mutex.unlock(); return dbgstream() << qPrintable( ind + AMK_PREFIX ); }

    typedef kdbgstream DebugStream;

    #undef AMK_PREFIX

    typedef kndbgstream NoDebugStream;
}

using Debug::debug;
using Debug::warning;
using Debug::error;
using Debug::fatal;
using Debug::DebugStream;

/// Standard function announcer
#define DEBUG_FUNC_INFO { Debug::mutex.lock(); kDebug() << Debug::indent() << k_funcinfo; Debug::mutex.unlock(); }

/// Announce a line
#define DEBUG_LINE_INFO { Debug::mutex.lock(); kDebug() << Debug::indent() << k_funcinfo << "Line: " << __LINE__; Debug::mutex.unlock(); }

/// Convenience macro for making a standard Debug::Block
#define DEBUG_BLOCK Debug::Block uniquelyNamedStackAllocatedStandardBlock( __PRETTY_FUNCTION__ );

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
            if( !debugEnabled() ) return;

            mutex.lock();
            gettimeofday( &m_start, 0 );

            kDebug() << "BEGIN: " << label << "\n";
            Debug::modifieableIndent() += "  ";
            mutex.unlock();
        }

        ~Block()
        {
            if( !debugEnabled() ) return;

            mutex.lock();
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

            Debug::modifieableIndent().truncate( Debug::indent().length() - 2 );
            kDebug() << "END__: " << m_label
                      << " - Took " << qPrintable( QString::number( duration, 'g', 2 ) ) << "s\n";
            mutex.unlock();
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


#include <QVariant>

namespace Debug
{
    /**
     * @class Debug::List
     * @short You can pass anything to this and it will output it as a list
     *
     *     debug() << (Debug::List() << anInt << aString << aQStringList << aDouble) << endl;
     */

    typedef QList<QVariant> List;
}

#endif
