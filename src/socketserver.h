// Maintainer: Max Howell <max.howell@methylblue.com>, (C) 2004
// Copyright: See COPYING file that comes with this distribution
//

#ifndef VIS_SOCKETSERVER_H
#define VIS_SOCKETSERVER_H

#include <qstring.h>          //stack allocated
#include <qlistview.h>        //baseclass
#include <qserversocket.h>    //baseclass
#include <qsocketnotifier.h>  //baseclass

class QPoint;
class KProcess;


namespace amaroK
{
    class SocketServer : public QServerSocket
    {
    public:
        SocketServer( const QString &socketName, QObject *parent );
       ~SocketServer();

    protected:
        int      m_sockfd;
        QCString m_path;
    };
}


namespace Vis
{
    class SocketServer : public amaroK::SocketServer
    {
    public:
        SocketServer( QObject* );

        void newConnection( int );

        QCString path() const { return m_path; }
    };

    class SocketNotifier : public QSocketNotifier
    {
        Q_OBJECT

    public:
        SocketNotifier( int sockfd );

    private slots:
        void request( int );
    };

    class Selector : public QListView
    {
        Q_OBJECT
        friend class Item;
        Selector( QWidget *parent=0 );
        SocketServer *m_server;

    public:
        static Selector* instance();

        ///assigns pid/sockfd combo
        void mapPID( int, int );

        class Item : public QCheckListItem
        {
        public:
            Item( QListView *parent, const char *command, const QString &text, const QString &s2 )
                    : QCheckListItem( parent, text, QCheckListItem::CheckBox )
                    , m_proc( 0 )
                    , m_sockfd( -1 )
                    , m_command( command ) { setText( 1, s2 ); }
           ~Item();

            virtual void stateChange( bool state );

            KProcess   *m_proc;
            int         m_sockfd;
            const char *m_command;
        };

    private slots:
        void rightButton( QListViewItem*, const QPoint&, int );

    public slots:
        void processExited( KProcess* );
    };
} //namespace VIS

#endif
