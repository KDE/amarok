// Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright: See COPYING file that comes with this distribution
//

#ifndef VIS_SOCKETSERVER_H
#define VIS_SOCKETSERVER_H

//TODO move loader server into here too so this file isn't named so badly
//TODO use only one socket?

#include <klistview.h>        //baseclass
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
    
    static QWidget* instance();
    static Selector* m_instance;
    
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

signals:
    void configureVis( const QString& );    
    
private slots:
    void rightButton( QListViewItem*, const QPoint&, int );       

public slots:
    void processExited( KProcess* );
};


class SocketServer : public QServerSocket
{
Q_OBJECT

public:
    SocketServer( QObject* );
    void newConnection( int );

    static SocketServer* m_self;

private slots:
    void request( int );

public slots:
    void invokeConfig( const QString& );    
    
private:
    static bool m_ignoreState;

    int m_sockfd;
    QString m_configVis;
};

        
} //namespace VIS

#endif
