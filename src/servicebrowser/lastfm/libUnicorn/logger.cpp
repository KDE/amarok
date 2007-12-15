/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Erik Jaelevik, Last.fm Ltd <erik@last.fm>                          *
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

#define ASSERT_ON_QT_WARNINGS

#include "logger.h"

#include <QSysInfo>
#include <QFile>

using namespace std;


/******************************************************************************
    loggingMsgHandler
    
    Message handler for redirecting qDebug output
******************************************************************************/
static void
loggingMsgHandler(
    QtMsgType   type,
    const char* msg )
{
#ifdef QT_NO_DEBUG

    // Release build, redirect to log file
    switch (type)
    {
        case QtDebugMsg:
            Logger::GetLogger().JustOutputThisHack( msg );
            break;
        
        case QtWarningMsg:
            LOGL(Logger::Warning, msg);
            break;
        
        case QtCriticalMsg:
            LOGL(Logger::Critical, msg);
            break;
        
        case QtFatalMsg:
            LOGL(Logger::Critical, msg);
            Logger::GetLogger().mDefaultMsgHandler(type, msg);
            break;
    }    

#else

    // Debug build, use default handler
    QtMsgHandler& defHandler = Logger::GetLogger().mDefaultMsgHandler;

    switch (type)
    {
        case QtDebugMsg:
            defHandler(type, msg);
            break;

        case QtWarningMsg:
        case QtCriticalMsg:
            defHandler(type, msg);
            #ifdef ASSERT_ON_QT_WARNINGS
                Q_ASSERT(!"Qt warning, might be a good idea to fix this");
            #endif
            break;

        case QtFatalMsg:
            defHandler(type, msg);
            break;
    }    

#endif // QT_NO_DEBUG
}

/******************************************************************************
    defaultMsgHandler

    On Mac (and Linux?), Qt doesn't give us a previously installed message
    handler on calling qInstallMsgHandler, so we'll give it this one instead.
    This code is pretty much copied from qt_message_output which isn't ideal
    but there was no way of falling back on calling that function in the
    default case as it would lead to infinite recursion.
******************************************************************************/
static void
defaultMsgHandler(
    QtMsgType   type,
    const char* msg )
{
    fprintf(stderr, "%s\n", msg);
    fflush(stderr);

    if (type == QtFatalMsg ||
       (type == QtWarningMsg && (!qgetenv("QT_FATAL_WARNINGS").isNull())) )
    {
        #if defined(Q_OS_UNIX) && defined(QT_DEBUG)
            abort(); // trap; generates core dump
        #else
            exit(1);
        #endif
    }
}

/******************************************************************************
    Init
******************************************************************************/
void
Logger::Init(
    QString sFilename,
    bool bOverwrite)
{
    mFilePath = sFilename;

    // Trim file size if above 500k
    QFile qf( sFilename );
    if ( qf.size() > 500000 )
    {
        ifstream inFile( qPrintable(sFilename) );
        inFile.seekg( static_cast<streamoff>( qf.size() - 400000 ) );
        istreambuf_iterator<char> bufReader( inFile ), end;
        string sFile;
        sFile.reserve( 400005 );
        sFile.assign( bufReader, end );
        inFile.close();
        ofstream outFile( qPrintable(sFilename) );
        outFile << sFile << flush;
        outFile.close();
    }

    ios::openmode flags = ios::out;
    if (!bOverwrite)
    {
        flags |= ios::app;
    }
    mFileOut.open(qPrintable(sFilename), flags);

    if (!mFileOut)
    {
        qWarning() << "Could not open log file" << sFilename;
        return;
    }

    SetLevel(Warning);

    // Print some initial startup info
    QString osVer = GetOSVersion();
    LOG(1, "************************************* STARTUP ********************************************\n");
    LOG(1, "OS: " << osVer << "\n");

    // Install message handler for dealing with qDebug output
    mDefaultMsgHandler = qInstallMsgHandler(loggingMsgHandler);
    if (mDefaultMsgHandler == 0)
    {
        // This will happen on the Mac. (And on Linux?)
        LOGL(2, "No default message handler found, using our own." );
        mDefaultMsgHandler = defaultMsgHandler;
    }
}

/******************************************************************************
    GetLogger
******************************************************************************/
Logger&
Logger::GetLogger()
{
    // This does only construct one instance of the logger object even if
    // called from lots of different modules. Yay!
    static Logger logger;
    return logger;
}

/******************************************************************************
    Log
******************************************************************************/
void
Logger::Log(
    Severity    level,
    string message,
    string function,
    int line)
{
    QMutexLocker loggerLock(&mMutex);    
    if (mFileOut && level <= GetLevel())
    {                                           
        mFileOut <<                          
            GetTime() << " - " <<            
            std::setw(4) << QThread::currentThreadId() << " - " <<           
            function << "(" << line << ") - " <<                                            
            "L" << level << "\n  " << message << std::endl;                      
    }                                                                        

}
    
/******************************************************************************
    JustOutputThisHack
******************************************************************************/

void
Logger::JustOutputThisHack(
    const char* msg)
{
    QMutexLocker loggerLock(&mMutex);
    if (mFileOut)
    {
        mFileOut << msg << "\n" << std::endl;
    }
}

    
/******************************************************************************
    GetOSVersion
******************************************************************************/
QString
Logger::GetOSVersion()
{
    QString version;

    #ifdef Q_WS_WIN
    switch (QSysInfo::WindowsVersion)
    {
        case QSysInfo::WV_32s:        version = "Windows 3.1 with Win32s"; break;
        case QSysInfo::WV_95:         version = "Windows 95"; break;
        case QSysInfo::WV_98:         version = "Windows 98"; break;
        case QSysInfo::WV_Me:         version = "Windows Me"; break;
        case QSysInfo::WV_DOS_based:  version = "MS-DOS-based Windows"; break;

        case QSysInfo::WV_NT:         version = "Windows NT"; break;
        case QSysInfo::WV_2000:       version = "Windows 2000"; break;
        case QSysInfo::WV_XP:         version = "Windows XP"; break;
        case QSysInfo::WV_2003:       version = "Windows Server 2003"; break;
        case QSysInfo::WV_VISTA:      version = "Windows Vista"; break;
        case QSysInfo::WV_NT_based:   version = "NT-based Windows"; break;

        case QSysInfo::WV_CE:         version = "Windows CE"; break;
        case QSysInfo::WV_CENET:      version = "Windows CE.NET"; break;
        case QSysInfo::WV_CE_based:   version = "CE-based Windows"; break;

        default:                      version = "Qt doesn't know"; break;
    }
    #elif defined Q_WS_MAC
    switch (QSysInfo::MacintoshVersion)
    {
        case QSysInfo::MV_Unknown:    version = "Unknown Mac"; break;
        case QSysInfo::MV_9:          version = "Mac OS 9"; break;
        case QSysInfo::MV_10_0:       version = "Mac OS X 10.0"; break;
        case QSysInfo::MV_10_1:       version = "Mac OS X 10.1"; break;
        case QSysInfo::MV_10_2:       version = "Mac OS X 10.2"; break;
        case QSysInfo::MV_10_3:       version = "Mac OS X 10.3"; break;
        case QSysInfo::MV_10_4:       version = "Mac OS X 10.4"; break;
        case QSysInfo::MV_10_5:       version = "Mac OS X 10.5"; break;
        
        default:                      version = "Qt doesn't know"; break;
    }
    #else
    version = "Unix/Linux";
    #endif // Q_WS_WIN

    return version;
}
