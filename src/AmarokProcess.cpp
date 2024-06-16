/****************************************************************************************
 * Copyright (c) 2007 Shane King <kde@dontletsstart.com>                                *
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

#include "AmarokProcess.h"    
#include "core/support/Debug.h"

#include <QTextCodec>

#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

AmarokProcess::AmarokProcess(QObject *parent) 
    : KProcess(parent), lowPriority(false) 
{
    connect( this, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
             this, QOverload<>::of(&AmarokProcess::finished) );
    connect( this, &QProcess::readyReadStandardOutput, this, &AmarokProcess::readyReadStandardOutput );
    connect( this, &QProcess::readyReadStandardError, this, &AmarokProcess::readyReadStandardError );
}

/** 
 * Due to xine-lib, we have to make KProcess close all fds, otherwise we get "device is busy" messages
 * exploiting setupChildProcess(), a virtual method that
 * happens to be called in the forked process
 * See bug #103750 for more information.
 */
void
AmarokProcess::setupChildProcess()
{
    KProcess::setupChildProcess();

#ifdef Q_OS_UNIX
    // can't get at the fds that QProcess needs to keep around to do its status
    // tracking , but fortunately it sets them to close on exec anyway, so if
    // we do likewise to everything then we should be ok.
    for(int i = sysconf(_SC_OPEN_MAX) - 1; i > 2; i--)
        fcntl(i, F_SETFD, FD_CLOEXEC);

    if( lowPriority )
        setpriority( PRIO_PROCESS, 0, 19 );
#endif
}

void
AmarokProcess::start()
{
    KProcess::start();

#ifdef Q_OS_WIN32
    if( lowPriority )
        SetPriorityClass( QProcess::pid()->hProcess, IDLE_PRIORITY_CLASS );
#endif
}

void
AmarokProcess::finished() // SLOT
{
    Q_EMIT processExited( this );
}

void
AmarokProcess::readyReadStandardOutput() // SLOT
{
    Q_EMIT receivedStdout( this );
}

void
AmarokProcess::readyReadStandardError() // SLOT
{
    Q_EMIT receivedStderr( this );
}

// AmarokProcIO
AmarokProcIO::AmarokProcIO ( QObject *parent )
    : AmarokProcess( parent ), codec( QTextCodec::codecForName( "UTF-8" ) )
{
}

bool 
AmarokProcIO::writeStdin (const QString &line)
{
    return write( codec->fromUnicode( line + '\n' ) ) > 0;
}

int 
AmarokProcIO::readln (QString &line)
{
    QByteArray bytes = readLine();
    if (bytes.length() == 0)
    {
        return -1;
    }
    else
    {
        // convert and remove \n
        line = codec->toUnicode( bytes.data(), bytes.length() - 1);
        return line.length();
    }
}

void
AmarokProcIO::start()
{
    connect (this, &AmarokProcIO::readyReadStandardOutput, this, &AmarokProcIO::readyReadStandardOutput);

    KProcess::start ();
}

void 
AmarokProcIO::readyReadStandardOutput()
{
    if( canReadLine() )
        Q_EMIT readReady( this );
}

// AmarokShellProcess
AmarokShellProcess &
AmarokShellProcess::operator<<(const QString& arg)
{
    if( program().isEmpty() )
        setShellCommand( arg );
    else
        AmarokProcess::operator<<( arg );
    return *this;
}

AmarokShellProcess &
AmarokShellProcess::operator<<(const QStringList& args)
{
    for( const QString &arg : args )
        *this << arg;
    return *this;
}
