// Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright: See COPYING file that comes with this distribution
//

#ifndef VIS_SOCKETSERVER_H
#define VIS_SOCKETSERVER_H

//TODO move loader server into here too so this file isn't named so badly
//TODO use only one socket?

#include <qguardedptr.h>      
#include <qserversocket.h>    //baseclass
#include <klistview.h>        //baseclass

#include <vector>             //stack allocated

class QPoint;
class KProcess;

namespace Vis {

class Selector : public KListView
{
public:
    Selector()
    : KListView() { setWFlags( Qt::WDestructiveClose ); }
};

class SocketServer : public QServerSocket
{
Q_OBJECT

public:
    SocketServer( QObject* );
    void newConnection( int );

public slots:
    void showSelector();    
    
private slots:
    void request( int );
    
private:
    class VisListItem : public QCheckListItem {
        public:
            VisListItem( QListView* parent, const QString& text ) 
            : QCheckListItem( parent, text, QCheckListItem::CheckBox ) {};
            void stateChange( bool state );
    };

    struct VisItem {
        KProcess*    vis;
        QString      name;
    };
    
    static QGuardedPtr<Selector> lv;
    static std::vector<VisItem> m_visList;
    static bool m_ignoreState;
               
    int m_sockfd;
};

        
} //namespace VIS

#endif
