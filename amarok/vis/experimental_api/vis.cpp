// Author:    Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright: See COPYING file that comes with this distribution
//

#include <stdlib.h>
#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "vis.h"


using namespace amK;

Vis::Vis()
  : m_path( getenv( "HOME" ) )
{
    m_data[0].resize( 512 );
    m_data[1].resize( 512 );

    //TODO get path using "kde-config --type data"
    m_path += "/.kde/share/apps/amarok/visualization_socket";
}

int
Vis::tryConnect()
{
    //try to connect to the VisServer
    int fd = socket( AF_UNIX, SOCK_STREAM, 0 );

    if( fd != -1 )
    {

        struct sockaddr_un local;

        strcpy( &local.sun_path[0], m_path.c_str() );
        local.sun_family = AF_UNIX;

        if( ::connect( fd, (struct sockaddr*) &local, sizeof( local ) ) == -1 )
        {
            fd = -1;
        }
    }

    ::close( fd ); //TODO keep connection open

    return fd;
}

std::vector<short16>*
Vis::fetchPCM()
{
    const int nch = 1; //no of channels?
    int sockfd;
    int nbytes;

    sockfd = tryConnect();

    if( sockfd != -1 )
    {
        ::send( sockfd, "PCM", 4, 0 );
        nbytes = ::recv( sockfd, m_data, 1024, 0 );
        ::close( sockfd );
    }

    return m_data;
}
