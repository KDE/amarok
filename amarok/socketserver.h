// Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright: See COPYING file that comes with this distribution
//

#ifndef VIS_SOCKETSERVER_H
#define VIS_SOCKETSERVER_H

//TODO move loader server into here too so this file isn't named so badly
//TODO use only one socket?

#include <klistview.h>        //baseclass
#include <qguardedptr.h>      //stack allocated
#include <qserversocket.h>    //baseclass
#include <vector>             //stack allocated

class QPoint;
class KProcess;


namespace Vis {

class Selector : public KListView
{
Q_OBJECT

public:
    Selector();
    
    static QWidget *instance() { return m_instance ? m_instance : (QWidget*)new Selector(); }
    static QGuardedPtr<Selector> m_instance;
    
    class Item : public QCheckListItem //TODO use stack allocated KProcess
    {
    public:
        Item( QListView *parent, const QString &text ) 
          : QCheckListItem( parent, text, QCheckListItem::CheckBox )
        {}
        ~Item();
        
        virtual void stateChange( bool state );
        
        KProcess *m_proc;
        
    };

public slots:
    void processExited( KProcess* );
};


class SocketServer : public QServerSocket
{
Q_OBJECT

public:
    SocketServer( QObject* );
    void newConnection( int );

private slots:
    void request( int );
    
private:
    static bool m_ignoreState;
               
    int m_sockfd;
};

        
} //namespace VIS

#endif
