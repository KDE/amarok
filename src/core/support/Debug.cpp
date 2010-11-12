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

#include "core/support/Debug.h"
#include "core/support/Debug_p.h"

#include "shared/amarok_export.h"

#include <KConfigGroup>
#include <KCmdLineArgs>
#include <KGlobal>

#include <QApplication>
#include <QMutex>
#include <QObject>

#include <iostream>

// Define Application wide prefix
#ifndef APP_PREFIX
#define APP_PREFIX QLatin1String( "amarok" )
#endif

#define DEBUG_INDENT_OBJECTNAME QLatin1String("Debug_Indent_object")

AMAROK_CORE_EXPORT QMutex Debug::mutex( QMutex::Recursive );

using namespace Debug;

static bool s_debugEnabled = false;
static bool s_debugColorsEnabled = false;

IndentPrivate::IndentPrivate(QObject* parent)
    : QObject(parent)
{
    setObjectName( DEBUG_INDENT_OBJECTNAME );
}

/**
 * We can't use a statically instantiated QString for the indent, because
 * static namespaces are unique to each dlopened library. So we piggy back
 * the QString on the KApplication instance
 */
IndentPrivate* IndentPrivate::instance()
{
    QObject* qOApp = reinterpret_cast<QObject*>(qApp);
    QObject* obj = qOApp ? qOApp->findChild<QObject*>( DEBUG_INDENT_OBJECTNAME ) : 0;
    return (obj ? static_cast<IndentPrivate*>( obj ) : new IndentPrivate( qApp ));
}

static int s_colors[] = { 1, 2, 4, 5, 6 }; // no yellow and white for sanity
static int s_colorIndex = 0;

static QString toString( DebugLevel level )
{
    switch( level )
    {
        case KDEBUG_WARN:
            return "[WARNING]";
        case KDEBUG_ERROR:
            return "[ERROR__]";
        case KDEBUG_FATAL:
            return "[FATAL__]";
        default:
            return QString();
    }
}

static QString colorize( const QString &text, int color = s_colorIndex )
{
    if( !debugColorEnabled() )
        return text;

    return QString( "\x1b[00;3%1m%2\x1b[00;39m" ).arg( QString::number(s_colors[color]), text );
}

static QString reverseColorize( const QString &text, int color )
{
    if( !debugColorEnabled() )
        return text;

    return QString( "\x1b[07;3%1m%2\x1b[00;39m" ).arg( QString::number(color), text );
}

const QString& Debug::indent()
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

kdbgstream Debug::dbgstream( DebugLevel level )
{
    if( !debugEnabled() )
        return kDebugDevNull();

    mutex.lock();
    const QString &currentIndent = indent();
    mutex.unlock();

    QString text = QString("%1%2")
        .arg( APP_PREFIX )
        .arg( currentIndent );
    if ( level > KDEBUG_INFO )
        text.append( ' ' + reverseColorize( toString(level), level ) );

    return kdbgstream( QtDebugMsg )
        << qPrintable( text );
}

void Debug::perfLog( const QString &message, const QString &func )
{
#ifdef Q_OS_UNIX
    if( !debugEnabled() )
        return;

    QString str = QString( "MARK: %1: %2 %3" ).arg( KCmdLineArgs::appName(), func, message );
    access( str.toLocal8Bit().data(), F_OK );
#endif
}

BlockPrivate::BlockPrivate( const char *text )
    : label( text )
    , color( s_colorIndex )
{
    if( !debugEnabled() )
        return;

#if QT_VERSION >= 0x040700
    startTime.start();
#else
    startTime = QTime::currentTime();
#endif

    mutex.lock();
    s_colorIndex = (s_colorIndex + 1) % 5;
    dbgstream()
        << qPrintable( colorize( QLatin1String( "BEGIN:" ), color ) )
        << label;
    IndentPrivate::instance()->m_string += QLatin1String("  ");
    mutex.unlock();
}

Block::Block( const char *label )
    : d( debugEnabled() ? new BlockPrivate( label ) : 0 )
{
}

Block::~Block()
{
    if( !d )
        return;

#if QT_VERSION >= 0x040700
    const double duration = d->startTime.elapsed() / 1000.0;
#else
    const double duration = (double)d->startTime.msecsTo( QTime::currentTime() ) / 1000.0;
#endif

    mutex.lock();
    IndentPrivate::instance()->m_string.truncate( Debug::indent().length() - 2 );
    mutex.unlock();

    // Print timing information, and a special message (DELAY) if the method took longer than 5s
    if( duration < 5.0 )
        dbgstream()
            << qPrintable( colorize( QLatin1String( "END__:" ), d->color ) )
            << d->label
            << qPrintable( colorize( QString( "[Took: %3s]").arg( QString::number(duration, 'g', 2) ), d->color ) );
    else
        dbgstream()
            << qPrintable( colorize( QString( "END__:" ), d->color ) )
            << d->label
            << qPrintable( reverseColorize( QString( "[DELAY Took (quite long) %3s]").arg( QString::number(duration, 'g', 2) ), KDEBUG_WARN ) );

    delete d;
}

void Debug::stamp()
{
    static int n = 0;
    debug() << "| Stamp: " << ++n << endl;
}
