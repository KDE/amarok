/****************************************************************************************
 * Copyright (c) 2003-2005 Max Howell <max.howell@methylblue.com>                       *
 * Copyright (c) 2007-2009 Mark Kretschmann <kretschmann@kde.org>                       *
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

#include <KGlobal>
#include <KCmdLineArgs>
#include <KConfig>
#include <KConfigGroup>
#include <kdebug.h>
#include <QApplication>
#include <QMutex>
#include <QObject>
#include <QTime>

#include <iostream>

#include "shared/amarok_export.h"

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
    extern AMAROK_CORE_EXPORT QMutex mutex;

    // we can't use a statically instantiated QString for the indent, because
    // static namespaces are unique to each dlopened library. So we piggy back
    // the QString on the KApplication instance

    #define qOApp reinterpret_cast<QObject*>(qApp)
    class Indent : QObject
    {
        friend QString &modifieableIndent();
        Indent() : QObject( qOApp ) { setObjectName( QLatin1String("DEBUG_indent") ); }
        QString m_string;
    };

    inline QString &modifieableIndent()
    {
        QObject* o = qOApp ? qOApp->findChild<QObject*>( QLatin1String("DEBUG_indent") ) : 0;
        QString &ret = (o ? static_cast<Indent*>( o ) : new Indent)->m_string;
        return ret;
    }

    inline QString indent()
    {
        return modifieableIndent();
    }

    inline bool debugEnabled()
    {
        return KGlobal::config()->group( QLatin1String("General") ).readEntry( QLatin1String("Debug Enabled"), false );
    }

    inline kdbgstream dbgstream()
    {
        return debugEnabled() ? kdbgstream( QtDebugMsg ) : kDebugDevNull();
    }
 
    #undef qOApp

    #ifndef DEBUG_PREFIX
    #define AMK_PREFIX ""
    #else
    #define AMK_PREFIX "[" DEBUG_PREFIX "]"
    #endif

    //from kdebug.h
    enum DebugLevels {
        KDEBUG_INFO  = 0,
        KDEBUG_WARN  = 1,
        KDEBUG_ERROR = 2,
        KDEBUG_FATAL = 3
    };

    static int s_colors[] = { 1, 2, 4, 5, 6 }; // no yellow and white for sanity
    static int s_colorIndex = 0;

    static QString colorize( const QString &text, int color = s_colorIndex )
    {
        QString output( text );
        if( !qgetenv("KDE_COLOR_DEBUG").isEmpty() )
        {
            output.prepend( QString("amarok: \x1b[00;3%1m").arg(QString::number(s_colors[color])) );
            output.append( QLatin1String("\x1b[00;39m") );
            return output;
        }
        output.prepend( QLatin1String("amarok: ") );
        return output;
    }

    static QString reverseColorize( const QString &text, int color )
    {
        if( !qgetenv("KDE_COLOR_DEBUG").isEmpty() )
            return QString("\x1b[07;3%1m%2\x1b[00;39m").arg(QString::number(color), text);
        else
            return text;
    }

    #define GET_INDENT mutex.lock(); const QString &text = colorize( indent() + AMK_PREFIX ); mutex.unlock();
    static inline kdbgstream debug()   { GET_INDENT; return dbgstream() << text.toLocal8Bit().constData(); }
    static inline kdbgstream warning() { GET_INDENT; return dbgstream() << text.toLocal8Bit().constData() << reverseColorize(QLatin1String(" [WARNING!]"), 3).toLocal8Bit().constData(); }
    static inline kdbgstream error()   { GET_INDENT; return dbgstream() << text.toLocal8Bit().constData() << reverseColorize(QLatin1String(" [ERROR!]"), 1).toLocal8Bit().constData(); }
    static inline kdbgstream fatal()   { GET_INDENT; return dbgstream() << text.toLocal8Bit().constData(); }
    #undef GET_INDENT

    #undef AMK_PREFIX

    static inline void perfLog( const QString &message, const QString &func )
    {
#ifdef Q_OS_UNIX
        if( !debugEnabled() )
        {
            return;
        }
        QString str = QString( "MARK: %1: %2 %3" ).arg( KCmdLineArgs::appName(), func, message );
        access( str.toLocal8Bit().data(), F_OK );
#endif
    }
}

using Debug::debug;
using Debug::warning;
using Debug::error;
using Debug::fatal;

/// Standard function announcer
#define DEBUG_FUNC_INFO { Debug::mutex.lock(); kDebug() << Debug::indent() ; Debug::mutex.unlock(); }

/// Announce a line
#define DEBUG_LINE_INFO { Debug::mutex.lock(); kDebug() << Debug::indent() << "Line: " << __LINE__; Debug::mutex.unlock(); }

#ifdef __SUNPRO_CC
#define __PRETTY_FUNCTION__ __FILE__
#endif

/// Convenience macro for making a standard Debug::Block
#define DEBUG_BLOCK Debug::Block uniquelyNamedStackAllocatedStandardBlock( __PRETTY_FUNCTION__ );

/// Use this to remind yourself to finish the implementation of a function
#define AMAROK_NOTIMPLEMENTED warning() << "NOT-IMPLEMENTED: " << __PRETTY_FUNCTION__ << endl;

/// Use this to alert other developers to stop using a function
#define AMAROK_DEPRECATED warning() << "DEPRECATED: " << __PRETTY_FUNCTION__ << endl;

/// Performance logging
#define PERF_LOG( msg ) { Debug::perfLog( msg, __PRETTY_FUNCTION__ ); }

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
        QTime m_startTime;
        const char *m_label;
        int m_color;

    public:
        Block( const char *label ) : m_label( label ), m_color( s_colorIndex )
        {
            m_startTime = QTime::currentTime();

            if( !debugEnabled() )
                return;

            mutex.lock();
            s_colorIndex = (s_colorIndex + 1) % 5;
            dbgstream() << colorize( QString( "%1BEGIN: %2" ).arg( indent(), QString::fromLatin1(label) ), m_color ).toLocal8Bit().constData();
            Debug::modifieableIndent() += QLatin1String("  ");
            mutex.unlock();
        }

        ~Block()
        {
            if( !debugEnabled() )
                return;

            mutex.lock();

            const double duration = (double)m_startTime.msecsTo( QTime::currentTime() ) / (double)1000.0;

            Debug::modifieableIndent().truncate( Debug::indent().length() - 2 );

            // Print timing information, and a special message (DELAY) if the method took longer than 5s
            if( duration < 5.0 )
                dbgstream() << colorize( QString( "%1END__: %2 - Took %3s" ).arg( indent(), QString::fromLatin1(m_label), QString::number(duration, 'g', 2) ), m_color ).toLocal8Bit().constData();
            else
                dbgstream() << colorize( QString( "%1END__: %2 - DELAY Took (quite long) %3s" ).arg( indent(), QString::fromLatin1(m_label), QString::number(duration, 'g', 2) ), m_color ).toLocal8Bit().constData();

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
