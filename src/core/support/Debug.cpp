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

#include "amarok_export.h"
#include "core/support/Debug.h"
#include "core/support/Debug_p.h"

#include <KConfigCore/KConfigGroup>

#include <QApplication>
#include <QRecursiveMutex>
#include <QObject>

#include <iostream>
#include <unistd.h>

// Define Application wide prefix
#ifndef APP_PREFIX
#define APP_PREFIX QLatin1String( "amarok:" )
#endif

#define DEBUG_INDENT_OBJECTNAME QLatin1String("Debug_Indent_object")

AMAROKCORE_EXPORT QRecursiveMutex Debug::mutex;

using namespace Debug;

static bool s_debugEnabled = false;
static bool s_debugColorsEnabled = false;

Q_GLOBAL_STATIC( NoDebugStream, s_noDebugStream )

IndentPrivate::IndentPrivate(QObject* parent)
    : QObject(parent)
{
    setObjectName( DEBUG_INDENT_OBJECTNAME );
}

/**
 * We can't use a statically instantiated QString for the indent, because
 * static namespaces are unique to each dlopened library. So we piggy back
 * the QString on the QApplication instance
 */
IndentPrivate* IndentPrivate::instance()
{
    QObject* qOApp = reinterpret_cast<QObject*>(qApp);
    QObject* obj = qOApp ? qOApp->findChild<QObject*>( DEBUG_INDENT_OBJECTNAME ) : nullptr;
    return (obj ? static_cast<IndentPrivate*>( obj ) : new IndentPrivate( qApp ));
}

/*
  Text color codes (use last digit here)
  30=black 31=red 32=green 33=yellow 34=blue 35=magenta 36=cyan 37=white
*/
static int s_colors[] = { 1, 2, 4, 5, 6 }; // no yellow and white for sanity
static int s_colorIndex = 0;

static QString toString( DebugLevel level )
{
    switch( level )
    {
        case KDEBUG_WARN:
            return QStringLiteral("[WARNING]");
        case KDEBUG_ERROR:
            return QStringLiteral("[ERROR__]");
        case KDEBUG_FATAL:
            return QStringLiteral("[FATAL__]");
        default:
            return QString();
    }
}

static int toColor( DebugLevel level )
{
    switch( level ) {
        case KDEBUG_WARN:
            return 3; // red
        case KDEBUG_ERROR:
        case KDEBUG_FATAL:
            return 1; // yellow
        default:
            return 0; // default: black
    }
}

static QString colorize( const QString &text, int color = s_colorIndex )
{
    if( !debugColorEnabled() )
        return text;

    return QStringLiteral( "\x1b[00;3%1m%2\x1b[00;39m" ).arg( QString::number(s_colors[color]), text );
}

static QString reverseColorize( const QString &text, int color )
{
    if( !debugColorEnabled() )
        return text;

    return QStringLiteral( "\x1b[07;3%1m%2\x1b[00;39m" ).arg( QString::number(color), text );
}

QString Debug::indent()
{
    return IndentPrivate::instance()->m_string;
}

bool Debug::debugEnabled()
{
    return s_debugEnabled;
}

bool Debug::debugColorEnabled()
{
    return s_debugColorsEnabled;
}

void Debug::setDebugEnabled( bool enable )
{
    s_debugEnabled = enable;
}

void Debug::setColoredDebug( bool enable )
{
    s_debugColorsEnabled = enable;
}

QDebug Debug::dbgstream( DebugLevel level )
{
    if( !debugEnabled() )
        return QDebug( s_noDebugStream );

    mutex.lock();
    const QString currentIndent = indent();
    mutex.unlock();

    QString text = QStringLiteral("%1%2").arg( APP_PREFIX ).arg( currentIndent );
    if ( level > KDEBUG_INFO )
        text.append( QLatin1Char( ' ' ) + reverseColorize( toString(level), toColor( level ) ) );

    return QDebug( QtDebugMsg ) << qPrintable( text );
}

void Debug::perfLog( const QString &message, const QString &func )
{
#ifdef Q_OS_UNIX
    if( !debugEnabled() )
        return;

    QString str = QStringLiteral( "MARK: %1: %2 %3" ).arg( qApp->applicationName(), func, message );
    access( str.toLocal8Bit().data(), F_OK );
#endif
}

Block::Block( const char *label )
    : m_label( label )
    , m_color( s_colorIndex )
{
    if( !debugEnabled() )
        return;

    m_startTime.start();

    mutex.lock();
    s_colorIndex = (s_colorIndex + 1) % 5;
    dbgstream()
        << qPrintable( colorize( QLatin1String( "BEGIN:" ), m_color ) )
        << m_label;
    IndentPrivate::instance()->m_string += QLatin1String("  ");
    mutex.unlock();
}

Block::~Block()
{
    if( !debugEnabled() )
        return;

    double duration = m_startTime.elapsed() / 1000.0;

    mutex.lock();
    IndentPrivate::instance()->m_string.truncate( Debug::indent().length() - 2 );
    mutex.unlock();

#ifdef DEBUG_OVERRIDE_ELAPSED_TIME
    duration = DEBUG_OVERRIDE_ELAPSED_TIME;
#endif
    // Print timing information, and a special message (DELAY) if the method took longer than 5s
    if( duration < 5.0 )
    {
        dbgstream()
            << qPrintable( colorize( QLatin1String( "END__:" ), m_color ) )
            << m_label
            << qPrintable( colorize( QString( "[Took: %3s]")
                                     .arg( QString::number(duration, 'g', 2) ), m_color ) );
    }
    else
    {
        dbgstream()
            << qPrintable( colorize( QString( "END__:" ), m_color ) )
            << m_label
            << qPrintable( reverseColorize( QString( "[DELAY Took (quite long) %3s]")
                                            .arg( QString::number(duration, 'g', 2) ), toColor( KDEBUG_WARN ) ) );
    }
}

void Debug::stamp()
{
    static int n = 0;
    debug() << "| Stamp: " << ++n << Qt::endl;
}
