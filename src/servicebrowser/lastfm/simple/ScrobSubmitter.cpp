/***************************************************************************
 *   Copyright (C) 2005 - 2007 by                                          *
 *      Last.fm Ltd <client@last.fm>                                       *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.         *
 ***************************************************************************/

// Stops warnings about deprecated CRT functions
#ifndef _CRT_SECURE_NO_DEPRECATE
    #define _CRT_SECURE_NO_DEPRECATE
#endif
#ifndef _SCL_SECURE_NO_DEPRECATE
    #define _SCL_SECURE_NO_DEPRECATE
#endif

#include "ScrobSubmitter.h"
#include "EncodingUtils.h"

#include <windows.h>
#include <tchar.h>
#include <process.h>
//#include <shfolder.h>
#include <shellapi.h>

#include <iostream>
#include <sstream>
#include <string>

#include "amarok.h"

#ifndef _tcsclen
int _tcsclen(const TCHAR *str)
{
    int i = 0;
    while (*str)
    {
        ++i;
        ++str;
    }
    return i;
}
#endif

using namespace std;

/******************************************************************************
    Constants
******************************************************************************/
static const char* kVersion = "1.1.1";
static const char* kHost = "localhost";
static const int   kDefaultPort = 33367;
static const int   kPortsToStep = 5;
static const int   kLaunchWait = 60000; // in ms

/******************************************************************************
    ScrobSubmitter
******************************************************************************/
ScrobSubmitter::ScrobSubmitter() :
    mActualPort(kDefaultPort),
    mDoPortstepping(true),
    mNextId(0),
    mStopThread(false),
    mLaunchTime(0)
{
    InitializeCriticalSection(&mMutex);
    
    mRequestAvailable = CreateEvent( 
        NULL,         // no security attributes
        TRUE,         // manual reset event
        FALSE,        // initial state is non-signaled
        NULL);        // object name

    mExit = CreateEvent( 
        NULL,         // no security attributes
        TRUE,         // manual reset event
        FALSE,        // initial state is non-signaled
        NULL);        // object name
}

/******************************************************************************
   ~ScrobSubmitter
******************************************************************************/
ScrobSubmitter::~ScrobSubmitter()
{
    // Not using dtor for thread termination as we can't be sure the client
    // is still alive so the status callbacks might fail.

    DeleteCriticalSection(&mMutex);

    CloseHandle(mRequestAvailable);
    CloseHandle(mExit);
}

/******************************************************************************
   Term
******************************************************************************/
void
ScrobSubmitter::Term()
{
    // Signal socket thread to exit
    mStopThread = true;
    SetEvent(mExit);

    // Wait for it to die before leaving
    DWORD waitResult = WaitForSingleObject(mSocketThread, 5000);
    if ( waitResult == WAIT_TIMEOUT || waitResult == WAIT_FAILED )
    {
        ostringstream os;
        os << "Wait for thread termination " <<
            ( ( waitResult == WAIT_TIMEOUT ) ? "timed out. " : "failed. " ) <<
            "GetLastError() " << GetLastError();
        ReportStatus( -1, true, os.str() );
    }
    
    CloseHandle(mSocketThread);
}    

/******************************************************************************
    Init
******************************************************************************/
void
ScrobSubmitter::Init(
    const string& pluginId,
    StatusCallback callback,
    void* userData)
{
    mPluginId = pluginId;
    mpReportStatus = callback;
    mpUserData = userData;
        
    bool isAutoLaunchEnabled = true;

    // Read autolaunch option from registry
    HKEY regKeyAS;
    LONG lResult = RegOpenKeyEx(
        HKEY_CURRENT_USER,
        _T("Software\\Last.fm\\Client"),
        0,              // reserved
        KEY_READ,       // access mask
        &regKeyAS);

    if (lResult == ERROR_SUCCESS)
    {
        TCHAR acBoolAsString[10];
        DWORD nSize = 10;
        lResult = RegQueryValueEx(
            regKeyAS,                                 // key
            _T("LaunchWithMediaPlayer"),              // value to query
            NULL,                                     // reserved
            NULL,                                     // tells you what type the value is
            reinterpret_cast<LPBYTE>(acBoolAsString), // store result here
            &nSize);

        if (lResult == ERROR_SUCCESS)
        {
            isAutoLaunchEnabled = _tcscmp(acBoolAsString, _T("true")) == 0;
        }

        RegCloseKey(regKeyAS);
    }
    
    if ( isAutoLaunchEnabled )
    {
        LaunchClient();
    }

    // Start socket thread
    unsigned threadId;

    mSocketThread = reinterpret_cast<HANDLE>(_beginthreadex( 
        NULL,                           // security crap
        0,                              // stack size
        SendToASThreadMain,             // start function
        reinterpret_cast<void*>(this),  // argument to thread
        0,                              // run straight away
        &threadId));                    // thread id out
    
    if (mSocketThread == 0) // Error
    {
        ReportStatus(-1, true, strerror(errno));
    }

}

/******************************************************************************
    LaunchClient
******************************************************************************/
bool
ScrobSubmitter::LaunchClient()
{
    TCHAR acExe[MAX_PATH + 1];
    DWORD nPathSize = MAX_PATH + 1;
    string sPath;

    acExe[0] = _T('\0');

    // Open HKCU key
    HKEY regKeyAS;
    LONG lResult = RegOpenKeyEx(
        HKEY_CURRENT_USER,
        _T("Software\\Last.fm\\Client"),
        0,              // reserved
        KEY_READ,       // access mask
        &regKeyAS);
    
    if (lResult == ERROR_SUCCESS)
    {
        // Try and read exe path
        lResult = RegQueryValueEx(
            regKeyAS,                           // key
            _T("Path"),                         // value to query
            NULL,                               // reserved
            NULL,                               // tells you what type the value is
            reinterpret_cast<LPBYTE>(acExe),    // store result here
            &nPathSize);

        RegCloseKey(regKeyAS);
    }

    if ( _tcslen( acExe ) == 0 )
    {
        // Couldn't read path from HKCU, try HKLM
        lResult = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            _T("Software\\Last.fm\\Client"),
            0,              // reserved
            KEY_READ,       // access mask
            &regKeyAS);
    
        if (lResult == ERROR_SUCCESS)
        {
            nPathSize = MAX_PATH + 1;

            // Try and read exe path
            lResult = RegQueryValueEx(
                regKeyAS,                           // key
                _T("Path"),                         // value to query
                NULL,                               // reserved
                NULL,                               // tells you what type the value is
                reinterpret_cast<LPBYTE>(acExe),    // store result here
                &nPathSize);

            RegCloseKey(regKeyAS);
        }
    }

    if ( _tcslen( acExe ) == 0 )
    {
        ReportStatus(-1, true,
            "Couldn't read the Last.fm exe path from the registry.");
        return false;
    }

    // Work out what the app dir is
    TCHAR defaultDir[MAX_PATH + 1];
    _tcsncpy( defaultDir, acExe, _tcsclen( acExe ) + 1 ); // +1 for null terminator

    // Look for last backslash
    TCHAR* pos = _tcsrchr( defaultDir, _T('\\') );
    if ( pos == NULL )
    {
        // Path might also use forward slashes
        pos = _tcsrchr( defaultDir, _T('/') );

        if ( pos == NULL )
        {
            ReportStatus(-1, true,
                "Last.fm exe path from registry invalid.");
            return false;
        }
    }
    
    // Truncate so that defaultDir just contains the dir
    pos[0] = _T('\0');
    
    // Since the client doesn't allow multiple instances, we can just kick it off
    // regardless of whether it's running or not.
    HINSTANCE h = ShellExecute(
        NULL, _T("open"), acExe, _T("--tray"), defaultDir, SW_SHOWNORMAL);
        
    if (reinterpret_cast<int>(h) <= 32) // Error
    {
        // Invalid handle means it didn't launch
        ostringstream os;
        os << "Failed launching Last.fm client. ShellExecute error: " << h;
        ReportStatus(-1, true, os.str());
        return false;
    }
    
    ReportStatus(-1, false, "Launched Last.fm client.");
    
    // Store time of launch
    mLaunchTime = GetTickCount();
    
    return true;
}

/******************************************************************************
    Start
******************************************************************************/
int
ScrobSubmitter::Start(
    string artist,
    string track,
    string album,
    string mbId,
    int    length,
    string filename,
    Encoding encoding)
{
    if (encoding != UTF_8)
    {
        ConvertToUTF8(artist, encoding);
        ConvertToUTF8(track, encoding);
        ConvertToUTF8(album, encoding);
        ConvertToUTF8(mbId, encoding);
        ConvertToUTF8(filename, encoding);
    }

    ostringstream osCmd;
    osCmd << "START c=" << mPluginId        << "&" <<
                   "a=" << Escape(artist)   << "&" <<
                   "t=" << Escape(track)    << "&" <<
                   "b=" << Escape(album)    << "&" <<
                   "m=" << Escape(mbId)     << "&" <<
                   "l=" << length           << "&" <<
                   "p=" << Escape(filename) << endl;

    string cmd = osCmd.str();

    return SendToAS(cmd);
}

/******************************************************************************
    Stop
******************************************************************************/
int
ScrobSubmitter::Stop()
{
    ostringstream osCmd;
    osCmd << "STOP c=" << mPluginId << endl;
    string sCmd = osCmd.str();

    return SendToAS(sCmd);
}

/******************************************************************************
    Pause
******************************************************************************/
int
ScrobSubmitter::Pause()
{
    ostringstream osCmd;
    osCmd << "PAUSE c=" << mPluginId << endl;
    string sCmd = osCmd.str();

    return SendToAS(sCmd);
}

/******************************************************************************
    Resume
******************************************************************************/
int
ScrobSubmitter::Resume()
{
    ostringstream osCmd;
    osCmd << "RESUME c=" << mPluginId << endl;
    string sCmd = osCmd.str();

    return SendToAS(sCmd);
}

// FIXME: give functions verb names to make it clear what they do
/******************************************************************************
    BootstrapComplete
******************************************************************************/
int
ScrobSubmitter::BootstrapComplete( const WCHAR* username )
{
    char usernameUtf8[255];
    usernameUtf8[EncodingUtils::UnicodeToUtf8( username, wcslen( username ), usernameUtf8, 254 )] = '\0';
    ostringstream osCmd;
    osCmd << "BOOTSTRAP c=" << mPluginId << "&" << "u=" << usernameUtf8 << endl;

    return SendToAS( osCmd.str() );
}

/******************************************************************************
    GetVersion
******************************************************************************/
string
ScrobSubmitter::GetVersion()
{
    return kVersion;
}

/******************************************************************************
    GetLogPath
******************************************************************************/
string
ScrobSubmitter::GetLogPath()
{
    return std::string(Amarok::saveLocation("scroblog/").toLocal8Bit());
}

/******************************************************************************
    SendToAS
******************************************************************************/
int
ScrobSubmitter::SendToAS(
    const std::string& cmd)
{
    if (mPluginId == "")
    {
        ReportStatus(-1, true,
            "Init hasn't been called with a plugin ID");
        return -1;
    }

    EnterCriticalSection(&mMutex);

    // Push the cmd on the queue
    mRequestQueue.push_back(make_pair(++mNextId, cmd));

    // If queue was empty, signal socket thread
    if (mRequestQueue.size() == 1)
    {
        SetEvent(mRequestAvailable);
    }
    
    LeaveCriticalSection(&mMutex);
    
    return mNextId;
}

/******************************************************************************
    SendToASThread
******************************************************************************/
void
ScrobSubmitter::SendToASThread()
{
    // By giving mRequestAvailable the lower index, it will take priority
    // when calling WaitForMultipleObjects and both events are signalled.
    HANDLE eventArray[2] = { mRequestAvailable, mExit };

    while (!mStopThread)
    {
        DWORD signalledHandle = WaitForMultipleObjects(
            2, eventArray, FALSE, INFINITE);
        
        // Check if it's the exit event
        if ((signalledHandle - WAIT_OBJECT_0) == 1) { continue; }

        EnterCriticalSection(&mMutex);

        // Pick first request from queue
        pair<int, string> reqPair = mRequestQueue.front();
        mRequestQueue.pop_front();

        // This means we will eat all the requests in the queue before
        // waiting again
        if (mRequestQueue.size() == 0)
        {
            ResetEvent(mRequestAvailable);
        }

        LeaveCriticalSection(&mMutex);
        
        int nId = reqPair.first;
        string sCmd = reqPair.second;
        
        bool connected = ConnectToAS(nId);
        if (!connected) { continue; }
        
        string sResponse;
        try
        {
            mSocket.Send(sCmd);
            mSocket.Receive(sResponse);

            // Can't throw
            mSocket.ShutDown();

            if (sResponse.substr(0, 2) != "OK")
            {
                ReportStatus(nId, true, sResponse);
            }
            else
            {
                ReportStatus(nId, false, sResponse);
            }
        }
        catch (BlockingClient::NetworkException& e)
        {
            // Shut socket down and report error
            mSocket.ShutDown();
            ReportStatus(nId, true, e.what());
        }

    } // end while

    _endthreadex( 0 );
}

/******************************************************************************
    ConnectToAS
******************************************************************************/
bool
ScrobSubmitter::ConnectToAS(
    int reqId)
{
    try
    {
        mSocket.Connect(kHost, mActualPort);
    }
    catch (BlockingClient::NetworkException& e)
    {
        // Abort if thread has been told to stop
        if (mStopThread) return false;

        // It might be the case that the app simply hasn't had time to
        // open its socket yet. If so, let's sleep for a bit and retry.

        // GetTickCount wraps once every 49.7 days. But due to the nature of unsigned arithmetic,
        // this code works correctly if the return value wraps one time, i.e. we're fine as long
        // as the time between launch and the launchWait is less than 49.7 days.
        DWORD nowTime = GetTickCount();
        DWORD timeSinceLaunch = nowTime - mLaunchTime;

        if (timeSinceLaunch < kLaunchWait)
        {
            ostringstream os;
            os << "Connect failed, sent command too early after startup. "
                "Time since launch: " << timeSinceLaunch << ". Retrying...";
            ReportStatus(reqId, false, os.str().c_str());

            Sleep(1000);
            return ConnectToAS(reqId);
        }
        else if (mDoPortstepping)
        {
            // If after the waiting period we're still failing it might be because
            // the original port was busy. The client might have port-stepped. Let's
            // try and find its true port.
            if (mActualPort <= (kDefaultPort + kPortsToStep))
            {
                mActualPort++;

                ostringstream os;
                os << "Port stepping to port " << mActualPort;
                ReportStatus(reqId, false, os.str().c_str());

                return ConnectToAS(reqId);
            }
            else
            {
                // If the port stepping didn't solve the problem, we don't want to
                // try it again on each new connection attempt as it hogs the socket
                // thread.
                mActualPort = kDefaultPort;
                mDoPortstepping = false;
            }
        }
        
        ReportStatus(reqId, true, e.what());
        return false;
    }
    
    return true;
}

/******************************************************************************
    ReportStatus
******************************************************************************/
void
ScrobSubmitter::ReportStatus(
    int reqId,
    bool error,
    const string& msg)
{
    if (!mStopThread && (mpReportStatus != NULL))
    {
        (*mpReportStatus)(reqId, error, msg, mpUserData);
    }
}    

/******************************************************************************
    ConvertToUTF8
******************************************************************************/
void
ScrobSubmitter::ConvertToUTF8(
	string&  text,
	Encoding encoding)
{
    switch (encoding)
    {
        case ISO_8859_1:
        {
            // A UTF-8 string can be up to 4 times as big as the ANSI
            size_t nUtf8MaxLen = text.size() * 4 + 1;
            char* pcUtf8 = new char[nUtf8MaxLen];
            EncodingUtils::AnsiToUtf8(text.c_str(),
                                      pcUtf8,
                                      static_cast<int>(nUtf8MaxLen));
            text = pcUtf8;
            delete[] pcUtf8;
        }
        break;
        
    }
}	

/******************************************************************************
    Escape
******************************************************************************/
string&
ScrobSubmitter::Escape(
    string& text)
{
    string::size_type nIdx = text.find("&");
    while (nIdx != string::npos)
    {
        text.replace(nIdx, 1, "&&");
        nIdx = text.find("&", nIdx + 2);
    }

    return text;
}    
