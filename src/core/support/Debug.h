/*
    Copyright (c) 2003-2005 Max Howell <max.howell@methylblue.com>
    Copyright (c) 2007-2009 Mark Kretschmann <kretschmann@kde.org>
    Copyright (c) 2010-2011 Kevin Funk <krf@electrostorm.net>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef AMAROK_DEBUG_H
#define AMAROK_DEBUG_H

// We always want debug output available at runtime
#undef QT_NO_DEBUG_OUTPUT
#undef KDE_NO_DEBUG_OUTPUT

#include "core/amarokcore_export.h"
#include <QDebug>
#include <QRecursiveMutex>
#include <QVariant>

// BEGIN: DEBUG_ASSERT
/**
 * Debug helper to write "soft" assertions with escape statements more easily
 * If the assertions fails, a warning is printed containing the position
 * (file and line number) of the assert and the second parameter is evaluated.
 *
 * Usage: DEBUG_ASSERT(assertion, statement)
 *
 * (pseudo code *without* DEBUG_ASSERT)
 * \code
 * bool someMethod(T* pointer) {
 *   if (!pointer)
 *     qWarning() << "Warning pointer is null, aborting";
 *     return false;
 *   (...)
 *   return someBoolean;
 * }
 * \endcode
 *
 * (may be replaced by)
 * \code
 * bool someMethod(T* pointer) {
 *   DEBUG_ASSERT(pointer, return false)
 *   (...)
 *   return someBoolean;
 * }
 * \endcode
 *
 * \author Kevin Funk
 * \sa http://qt.gitorious.org/qt-creator/qt-creator/blobs/master/src/libs/utils/qtcassert.h
 */
#define DEBUG_ASSERT(cond, action) \
    if(cond){}else{warning()<< \
        "ASSERTION " #cond " FAILED AT " __FILE__ ":" DEBUG_ASSERT_STRINGIFY(__LINE__);action;}

#define DEBUG_ASSERT_STRINGIFY_INTERNAL(x) #x
#define DEBUG_ASSERT_STRINGIFY(x) DEBUG_ASSERT_STRINGIFY_INTERNAL(x)
// END__: DEBUG_ASSERT

# include <QElapsedTimer>

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
 *        debug() << "output1" << Qt::endl;
 *        debug() << "output2" << Qt::endl;
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
    extern AMAROKCORE_EXPORT QRecursiveMutex mutex;

    // from kdebug.h
    enum DebugLevel {
        KDEBUG_INFO  = 0,
        KDEBUG_WARN  = 1,
        KDEBUG_ERROR = 2,
        KDEBUG_FATAL = 3
    };

    AMAROKCORE_EXPORT QDebug dbgstream( DebugLevel level = KDEBUG_INFO );
    AMAROKCORE_EXPORT bool debugEnabled();
    AMAROKCORE_EXPORT bool debugColorEnabled();
    AMAROKCORE_EXPORT void setDebugEnabled( bool enable );
    AMAROKCORE_EXPORT void setColoredDebug( bool enable );
    AMAROKCORE_EXPORT QString indent();

    static inline QDebug dbgstreamwrapper( DebugLevel level ) {
#ifdef DEBUG_PREFIX
        return dbgstream( level ) << AMAROK_PREFIX;
#else
        return dbgstream( level );
#endif
    }

    static inline QDebug debug()   { return dbgstreamwrapper( KDEBUG_INFO ); }
    static inline QDebug warning() { return dbgstreamwrapper( KDEBUG_WARN ); }
    static inline QDebug error()   { return dbgstreamwrapper( KDEBUG_ERROR ); }
    static inline QDebug fatal()   { return dbgstreamwrapper( KDEBUG_FATAL ); }

    AMAROKCORE_EXPORT void perfLog( const QString &message, const QString &func );
}

using Debug::debug;
using Debug::warning;
using Debug::error;
using Debug::fatal;

/// Standard function announcer
#define DEBUG_FUNC_INFO { Debug::mutex.lock(); qDebug() << Debug::indent() ; Debug::mutex.unlock(); }

/// Announce a line
#define DEBUG_LINE_INFO { Debug::mutex.lock(); qDebug() << Debug::indent() << "Line: " << __LINE__; Debug::mutex.unlock(); }

/// Convenience macro for making a standard Debug::Block
#define DEBUG_BLOCK Debug::Block uniquelyNamedStackAllocatedStandardBlock( __PRETTY_FUNCTION__ );

/// Use this to remind yourself to finish the implementation of a function
#define AMAROK_NOTIMPLEMENTED warning() << "NOT-IMPLEMENTED:" << __PRETTY_FUNCTION__ << Qt::endl;

/// Use this to alert other developers to stop using a function
#define AMAROK_DEPRECATED warning() << "DEPRECATED:" << __PRETTY_FUNCTION__ << Qt::endl;

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
     *         debug() << "output1" << Qt::endl;
     *         debug() << "output2" << Qt::endl;
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
        AMAROKCORE_EXPORT explicit Block( const char *name );
        AMAROKCORE_EXPORT ~Block();

    private:
        QElapsedTimer m_startTime;

        const char *m_label;
        int m_color;
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
    AMAROKCORE_EXPORT void stamp();

    /**
     * @class Debug::List
     * @short You can pass anything to this and it will output it as a list
     *
     *     debug() << (Debug::List() << anInt << aString << aQStringList << aDouble) << Qt::endl;
     */

    typedef QList<QVariant> List;
}

#endif
