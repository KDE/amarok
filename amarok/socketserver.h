// Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright: See COPYING file that comes with this distribution
//

#ifndef VIS_SOCKETSERVER_H
#define VIS_SOCKETSERVER_H

//TODO move loader server into here too so this file isn't named so badly
//TODO use only one socket?

#include <qserversocket.h>

class QListViewItem;
class QPoint;


namespace Vis {

class SocketServer : public QServerSocket
{
Q_OBJECT
public:
    SocketServer( QObject* );
    void newConnection( int );

public slots:
    void showSelector();    
    
private slots:
    void visClicked( QListViewItem*, const QPoint&, int );
    void request( int );

private:
    int m_sockfd;
};

} //namespace VIS

#endif
