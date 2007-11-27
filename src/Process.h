/***************************************************************************
 * copyright            : (C) 2007 Shane King <kde@dontletsstart.com>      *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

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

class AMAROK_EXPORT Process : public KProcess
{
    Q_OBJECT

    public:
        explicit Process(QObject *parent = 0);

        void setLowPriority(bool lowPriority) { this->lowPriority = lowPriority; }

        void start();

    // for K3Process compat
    Q_SIGNALS:
        void processExited(Process *proc);
        void receivedStdout(Process *proc);
        void receivedStderr(Process *proc);

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

class AMAROK_EXPORT ProcIO : public Process
{
    Q_OBJECT

    public:
        ProcIO();

        int readln (QString &line);
        bool writeStdin(const QString &line);

        void start();

    Q_SIGNALS:
        void readReady(ProcIO *pio);
        
    private slots:
        void readyReadStandardOutput();
    
    private:
        QTextCodec *codec;
};

////////////////////////////////////////////////////////////////////////////////
// class ShellProcess
////////////////////////////////////////////////////////////////////////////////

class AMAROK_EXPORT ShellProcess : public Process
{
    public:
        explicit ShellProcess(QObject *parent = 0) : Process(parent) {}

        ShellProcess &operator<<(const QString& arg);
        ShellProcess &operator<<(const QStringList& args);
};

#endif // AMAROK_PROCESS_H
