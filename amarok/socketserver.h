// Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright: See COPYING file that comes with this distribution
//

#ifndef VIS_SOCKETSERVER_H
#define VIS_SOCKETSERVER_H


//TODO move loader server into here too so this file isn't named so badly
//TODO use only one socket?


#define AMK_NEW_VIS_SYSTEM

#include <qserversocket.h>

namespace Vis {

class SocketServer : public QServerSocket
{
public:
    SocketServer( QObject* );

    void newConnection( int );

private:
    int m_sockfd;
};

} //namespace VIS

#endif
