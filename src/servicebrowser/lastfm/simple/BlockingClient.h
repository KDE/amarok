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

#ifndef BLOCKINGCLIENT_H
#define BLOCKINGCLIENT_H

#include <winsock2.h>

#include <string>
#include <stdexcept>

/*************************************************************************/ /**
    Sockets-based client for sending and receiving commands down a socket
    using Winsock2.

    @author <erik@last.fm>
******************************************************************************/
class BlockingClient
{
public:

    /*************************************************************************
        Exception class for when things go wrong.
    **************************************************************************/
    class NetworkException : public std::logic_error {
    public:
        NetworkException(const std::string& sMsg) : logic_error(sMsg) { }
    };

    /*********************************************************************/ /**
        Ctor
    **************************************************************************/
	BlockingClient();

    /*********************************************************************/ /**
        Dtor
    **************************************************************************/
	virtual
	~BlockingClient();

    /*********************************************************************/ /**
        Connect to server.
        
        @param[in] sHost The host to connect to.
        @param[in] nPort The port on the host to connect to.
    **************************************************************************/
    void
    Connect(
        const std::string& sHost,
        u_short            nPort);

    /*********************************************************************/ /**
        Send a string down the socket.

        @param[in] sData The data to send.
    **************************************************************************/
    void
    Send(
        const std::string& sData);

    /*********************************************************************/ /**
        Read a line from the socket.

        @param[out] sLine String the received line is written to.
    **************************************************************************/
    void
    Receive(
        std::string& sLine);

    /*********************************************************************/ /**
        Shut down connection gracefully.
    **************************************************************************/
    void
    ShutDown();

private:
    
    /*********************************************************************/ /**
        Given an address string, determine if it's a dotted-quad IP address
        or a domain address. If the latter, ask DNS to resolve it. In either
        case, return resolved IP address. Throws CNetworkException on fail.
    **************************************************************************/
    u_long
    LookUpAddress(
        const std::string& sHost);

    /*********************************************************************/ /**
        Connects to a given address, on a given port, both of which must be
        in network byte order. Returns newly-connected socket if we succeed,
        or INVALID_SOCKET if we fail.
    **************************************************************************/
    SOCKET
    ConnectToSocket(
        u_long  nRemoteAddr,
        u_short nPort);


    SOCKET mSocket;
};

#endif // BLOCKINGCLIENT_H
