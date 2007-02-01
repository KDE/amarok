/***************************************************************************
 *   Copyright (C) 2004,5 Max Howell <max.howell@methylblue.com>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef VIS_SOCKETSERVER_H
#define VIS_SOCKETSERVER_H

#include <qstring.h>          //stack allocated
#include <q3listview.h>        //baseclass
#include <q3serversocket.h>    //baseclass
#include <qsocketnotifier.h>  //baseclass
//Added by qt3to4:
#include <QPaintEvent>
#include <Q3CString>

class QPoint;
class KProcess;


namespace Amarok
{
    class SocketServer : public Q3ServerSocket
    {
    public:
        SocketServer( const QString &socketName, QObject *parent );
       ~SocketServer();

    protected:
        int      m_sockfd;
        Q3CString m_path;
    };
}


namespace Vis
{
    class SocketServer : public Amarok::SocketServer
    {
    public:
        SocketServer( QObject* );

        void newConnection( int );

        Q3CString path() const { return m_path; }
    };

    class SocketNotifier : public QSocketNotifier
    {
        Q_OBJECT

    public:
        SocketNotifier( int sockfd );

    private slots:
        void request( int );
    };

    class Selector : public Q3ListView
    {
        Q_OBJECT
        Selector( QWidget *parent=0 );
        SocketServer *m_server;

        virtual void viewportPaintEvent( QPaintEvent* );

    public:
        static Selector* instance();
        class Item;
        friend class Item;

        ///assigns pid/sockfd combo
        void mapPID( int, int );

        class Item : public Q3CheckListItem
        {
        public:
            Item( Q3ListView *parent, const char *command, const QString &text, const QString &s2 )
                    : Q3CheckListItem( parent, text, Q3CheckListItem::CheckBox )
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
        void rightButton( Q3ListViewItem*, const QPoint&, int );

    public slots:
        void processExited( KProcess* );
        void receivedStdout( KProcess*, char*, int );
    };
} //namespace VIS

#endif
