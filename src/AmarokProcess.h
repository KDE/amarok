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

#ifndef AMAROK_PROCESS_H
#define AMAROK_PROCESS_H

#include "amarok_export.h"

#include <KProcess>

class QTextCodec;

// Classes needed to wrap some KProcess stuff to make it more like K3Process
// Also need to close fds on fork under unix

////////////////////////////////////////////////////////////////////////////////
// class Process
////////////////////////////////////////////////////////////////////////////////

class AMAROK_EXPORT AmarokProcess : public KProcess
{
    Q_OBJECT

    public:
        explicit AmarokProcess(QObject *parent = 0);

        void setLowPriority(bool lowPriority) { this->lowPriority = lowPriority; }

        void start();

    // for K3Process compat
    Q_SIGNALS:
        void processExited(AmarokProcess *proc);
        void receivedStdout(AmarokProcess *proc);
        void receivedStderr(AmarokProcess *proc);

    protected:
        virtual void setupChildProcess();

    private slots:
        void finished();
        void readyReadStandardOutput();
        void readyReadStandardError();

    private:
        bool lowPriority;
};

////////////////////////////////////////////////////////////////////////////////
// class ProcIO
////////////////////////////////////////////////////////////////////////////////

class AMAROK_EXPORT AmarokProcIO : public AmarokProcess
{
    Q_OBJECT

    public:
        explicit AmarokProcIO(QObject *parent = 0);

        int readln (QString &line);
        bool writeStdin(const QString &line);

        void start();

    Q_SIGNALS:
        void readReady(AmarokProcIO *pio);
        
    private slots:
        void readyReadStandardOutput();
    
    private:
        QTextCodec *codec;
};

////////////////////////////////////////////////////////////////////////////////
// class ShellProcess
////////////////////////////////////////////////////////////////////////////////

class AMAROK_EXPORT AmarokShellProcess : public AmarokProcess
{
    public:
        explicit AmarokShellProcess(QObject *parent = 0) : AmarokProcess(parent) {}

        AmarokShellProcess &operator<<(const QString& arg);
        AmarokShellProcess &operator<<(const QStringList& args);
};

#endif // AMAROK_PROCESS_H
