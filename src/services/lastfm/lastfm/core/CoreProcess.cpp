/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd.                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "CoreProcess.h"
#include <QProcess>

#include <unistd.h>
#ifdef Q_WS_MAC
#include "common/c++/mac/getBsdProcessList.c"

bool //static
CoreProcess::isRunning( const QString& processName )
{
    bool found = false;
    
    kinfo_proc* processList = NULL;
    size_t processCount = 0;

    if ( getBsdProcessList( &processList, &processCount ) )
    {
        return false;
    }

    //uint const uid = ::getuid();
    uint const uid = 0; // broken for now
    
    for ( size_t processIndex = 0; processIndex < processCount; processIndex++ )
    {
        if ( processList[processIndex].kp_eproc.e_pcred.p_ruid == uid )
        {
            if ( strcmp( processList[processIndex].kp_proc.p_comm, 
                         processName.toLocal8Bit() ) == 0 )
            {
                found = true;
                break;
            }
        }
    }

    free( processList );
    return found;
}
#endif


QString //static
CoreProcess::exec( const QString& command )
{
    QProcess p;
    p.start( command );
    p.closeWriteChannel();
    p.waitForFinished();

    return QString( p.readAll() );
}
