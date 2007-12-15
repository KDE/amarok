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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#include "BlockingClient.h"

#define USE_LOGGER (0)

#if USE_LOGGER
    #include "Logger.h"
#else
    #define LOG(x, y)
#endif

#ifdef MSVC
#include <crtdbg.h> // for _ASSERT
#else
#include <assert.h>
#define _ASSERT assert
#endif

#include <sstream> // for ostringstream

using namespace std;

/******************************************************************************
    Constants
******************************************************************************/
static const int   kBufferSize = 1024;

/******************************************************************************
    BlockingClient
******************************************************************************/
BlockingClient::BlockingClient() :
    mSocket(INVALID_SOCKET)
{
    // Start Winsock up
    WSAData wsaData;
	int nCode = WSAStartup(MAKEWORD(1, 1), &wsaData);
	
    if (nCode != 0)
    {
        int nErrorCode = WSAGetLastError();
        ostringstream osErr;
        osErr << "Winsock initialisation failed. Socket error: " << nErrorCode;
        throw NetworkException(osErr.str());
    }
}

/******************************************************************************
   ~BlockingClient
******************************************************************************/
BlockingClient::~BlockingClient()
{
    // Shut Winsock back down
    WSACleanup();
}

/******************************************************************************
    Connect
******************************************************************************/
void
BlockingClient::Connect(
    const string& sHost,
    u_short       nPort)
{
    // If anything in here throws, let it propagate up
    
    u_long nRemoteAddress = LookUpAddress(sHost);
    
    mSocket = ConnectToSocket(nRemoteAddress, nPort);
}

/******************************************************************************
    LookUpAddress
******************************************************************************/
u_long
BlockingClient::LookUpAddress(
    const string& sHost)
{
    u_long nRemoteAddr = inet_addr(sHost.c_str());

    if (nRemoteAddr == INADDR_NONE)
    {
        // Host isn't a dotted IP, so resolve it through DNS
        hostent* pHE = gethostbyname(sHost.c_str());
        if (pHE == NULL)
        {
            ostringstream osErr;
            osErr << "Lookup of hostname " << sHost << " failed.";
            throw NetworkException(osErr.str());
        }

        nRemoteAddr = *((u_long*)pHE->h_addr_list[0]);
    }

    return nRemoteAddr;
}


/******************************************************************************
    ConnectToSocket
******************************************************************************/
SOCKET
BlockingClient::ConnectToSocket(
    u_long  nRemoteAddr,
    u_short nPort)
{
    // Create a stream socket
    SOCKET sd = socket(AF_INET, SOCK_STREAM, 0);

    if (sd == INVALID_SOCKET)
    {
        ostringstream osErr;
        osErr << "Couldn't create socket. Socket error: " << WSAGetLastError();
        throw NetworkException(osErr.str());        
    }

    sockaddr_in sinRemote;
    sinRemote.sin_family = AF_INET;
    sinRemote.sin_addr.s_addr = nRemoteAddr;

    sinRemote.sin_port = htons(nPort);

    int nCode = connect(sd, (sockaddr*)&sinRemote, sizeof(sockaddr_in));
    
    if (nCode == SOCKET_ERROR)
    {
        ostringstream osErr;
        osErr << "Couldn't connect to server. Socket error: " << WSAGetLastError();
        closesocket(sd);
        throw NetworkException(osErr.str());        
    }

    return sd;
}

/******************************************************************************
    Send
******************************************************************************/
void
BlockingClient::Send(
    const string& sData)
{
    _ASSERT(mSocket != INVALID_SOCKET);
    
    int nBytesToSend = static_cast<int>(sData.size());
    int nTotalBytesSent = 0;

    while (nTotalBytesSent < nBytesToSend)
    {
        // Should never block
        int nBytesSent = send(mSocket,
                              sData.c_str(),
                              nBytesToSend - nTotalBytesSent,
                              0);

        if (nBytesSent == SOCKET_ERROR || nBytesSent == 0)
        {
            int nErrorCode = WSAGetLastError();
            ostringstream osErr;
            osErr << "Sending of data through socket failed. Socket error: " << nErrorCode;
            throw NetworkException(osErr.str());
        }

        // Send successful
        nTotalBytesSent += nBytesSent;
        
    } // end while
    
}


/******************************************************************************
    Receive
******************************************************************************/
void
BlockingClient::Receive(
    string& sLine)
{
    _ASSERT(mSocket != INVALID_SOCKET);

    // Read data from server.
    char acReadBuffer[kBufferSize];
    int nReadSize = kBufferSize - 1; // so that we can append a \0
    int nBytesRead;

    // Go into loop that keeps going until a CR is received
    bool bKeepGoing = true;
    int nSanityCtr = 0;
    do
    {
        // Will block until there is data available or timeout is reached
        nBytesRead = recv(mSocket, acReadBuffer, nReadSize, 0);

        if (nBytesRead == SOCKET_ERROR || nBytesRead == 0)
        {
            // This happens when something goes wrong, client dies or something.
            // Caller should close talk socket and recover.
            int nErrorCode = WSAGetLastError();
            ostringstream osErr;
            osErr << "Receiving data through socket failed. Socket error: " << nErrorCode;
            sLine = "";
            throw NetworkException(osErr.str());
        }

        // Read successful, is last char a CR?
        if (acReadBuffer[nBytesRead - 1] == '\n')
        {
            // Step back to before \n
            nBytesRead--;
            
            // If so, we're done
            bKeepGoing = false;
        }

        acReadBuffer[nBytesRead] = '\0';

        sLine += acReadBuffer;

        // I don't THINK this should ever happen but the documentaion for recv
        // is confusing so just to be on the safe side...
        if ( nSanityCtr++ > 10 ) bKeepGoing = false;

    } while (bKeepGoing);
    
}


/******************************************************************************
    ShutDown
******************************************************************************/
void
BlockingClient::ShutDown()
{
    // Disallow any further data sends. This will tell the other side
    // that we want to go away.
    int nResult = shutdown(mSocket, SD_SEND);

    if (nResult == SOCKET_ERROR)
    {
        // Even though this is an unforeseen failure, we don't want to
        // throw as we're shutting down. Just carry on shutting down
        // best we can.
        int nErrorCode = WSAGetLastError();
        ostringstream osErr;
        osErr << "Socket shutdown failed. Socket error: " << nErrorCode;
        LOG(1, osErr.str() << "\n");
    }

    // Receive any extra data still sitting on the socket. After all
    // data is received, this call will block until the remote host
    // acknowledges the TCP control packet sent by the shutdown above.
    // Then we'll get a 0 back from recv, signalling that the remote
    // host has closed its side of the connection.
    char acReadBuffer[kBufferSize];
    int nSanityCtr = 0;
    while (true)
    {
        int nBytesRead = recv(mSocket, acReadBuffer, kBufferSize, 0);

        if (nBytesRead == SOCKET_ERROR)
        {
            // I don't think we care here if the call failed.
            // Just carry on shutting down best we can.
            int nErrorCode = WSAGetLastError();
            ostringstream osErr;
            osErr << "Waiting for shutdown confirmation failed. Socket error: " << nErrorCode << "\n";
            LOG(1, osErr.str() << "\n");
            break;
        }
        else if (nBytesRead != 0)
        {
            string sTemp;
            sTemp.append(acReadBuffer, nBytesRead);
            ostringstream osErr;
            osErr << "Received " << nBytesRead <<
                     " unexpected bytes during shutdown:\n" << sTemp.c_str();
            LOG(1, osErr.str() << "\n");
            
            if ( nSanityCtr++ > 10 ) break;
        }
        else
        {
            // Returned 0, what we expect
            break;
        }
    }

    // Close the socket
    nResult = closesocket(mSocket);

    if (nResult == SOCKET_ERROR)
    {
        // Again, we don't want to throw. Log the error and continue.
        int nErrorCode = WSAGetLastError();
        ostringstream osErr;
        osErr << "Couldn't close socket. Socket error: " << nErrorCode;
        LOG(1, osErr.str() << "\n");
    }

}
