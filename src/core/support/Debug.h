/****************************************************************************************
 * Copyright (c) 2003-2005 Max Howell <max.howell@methylblue.com>                       *
 * Copyright (c) 2007-2009 Mark Kretschmann <kretschmann@kde.org>                       *
 * Copyright (c) 2010 Kevin Funk <krf@electrostorm.net>                                 *
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

#ifndef AMAROK_DEBUG_H
#define AMAROK_DEBUG_H

// We always want debug output available at runtime
#undef QT_NO_DEBUG_OUTPUT
#undef KDE_NO_DEBUG_OUTPUT

#include "shared/amarok_export.h"

#include <kdebug.h>

#include <QMutex>

// Platform specific macros
#ifdef _WIN32
#define __PRETTY_FUNCTION__ __FUNCTION__
#endif
#ifdef __SUNPRO_CC
#define __PRETTY_FUNCTION__ __FILE__
#endif

// Debug prefix, override if needed
#ifndef DEBUG_PREFIX
#define AMAROK_PREFIX ""
#else
#define AMAROK_PREFIX "[" DEBUG_PREFIX "]"
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
    extern AMAROK_CORE_EXPORT QMutex mutex;

    // from kdebug.h
    enum DebugLevel {
        KDEBUG_INFO  = 0,
        KDEBUG_WARN  = 1,
        KDEBUG_ERROR = 2,
        KDEBUG_FATAL = 3
    };

    AMAROK_CORE_EXPORT kdbgstream dbgstream( DebugLevel level = KDEBUG_INFO );
    AMAROK_CORE_EXPORT bool debugEnabled();
    AMAROK_CORE_EXPORT bool debugColorEnabled();
    AMAROK_CORE_EXPORT void setDebugEnabled( bool enable );
    AMAROK_CORE_EXPORT void setColoredDebug( bool enable );
    AMAROK_CORE_EXPORT QString indent();

    static inline kdbgstream dbgstreamwrapper( DebugLevel level ) {
#ifdef DEBUG_PREFIX
        return dbgstream( level ) << AMAROK_PREFIX;
#else
        return dbgstream( level );
#endif
    }

    static inline kdbgstream debug()   { return dbgstreamwrapper( KDEBUG_INFO ); }
    static inline kdbgstream warning() { return dbgstreamwrapper( KDEBUG_WARN ); }
    static inline kdbgstream error()   { return dbgstreamwrapper( KDEBUG_ERROR ); }
    static inline kdbgstream fatal()   { return dbgstreamwrapper( KDEBUG_FATAL ); }

    AMAROK_CORE_EXPORT void perfLog( const QString &message, const QString &func );
}

using Debug::debug;
using Debug::warning;
using Debug::error;
using Debug::fatal;

/// Standard function announcer
#define DEBUG_FUNC_INFO { Debug::mutex.lock(); kDebug() << Debug::indent() ; Debug::mutex.unlock(); }

/// Announce a line
#define DEBUG_LINE_INFO { Debug::mutex.lock(); kDebug() << Debug::indent() << "Line: " << __LINE__; Debug::mutex.unlock(); }

/// Convenience macro for making a standard Debug::Block
#define DEBUG_BLOCK Debug::Block uniquelyNamedStackAllocatedStandardBlock( __PRETTY_FUNCTION__ );

/// Use this to remind yourself to finish the implementation of a function
#define AMAROK_NOTIMPLEMENTED warning() << "NOT-IMPLEMENTED:" << __PRETTY_FUNCTION__ << endl;

/// Use this to alert other developers to stop using a function
#define AMAROK_DEPRECATED warning() << "DEPRECATED:" << __PRETTY_FUNCTION__ << endl;

/// Performance logging
#define PERF_LOG( msg ) { Debug::perfLog( msg, __PRETTY_FUNCTION__ ); }

class BlockPrivate;

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
    public:
        AMAROK_CORE_EXPORT Block( const char *name );
        AMAROK_CORE_EXPORT ~Block();

    private:
       BlockPrivate *const d;
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
    AMAROK_CORE_EXPORT void stamp();
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
