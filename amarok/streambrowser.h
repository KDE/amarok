//
//
// C++ Interface: $MODULE$
//
// Description: 
//
//
// Author: Max Howell <max.howell@methylblue.com>, (C) 2003
//
// Copyright: See COPYING file that comes with this distribution
//
//

#ifndef STREAMBROWSER_H
#define STREAMBROWSER_H

#include <qwidget.h>
#include <qsocket.h>
#include <qstringlist.h>

#include <klistview.h>


class QListViewItem;

/**
@author Max Howell

All code was ripped from KDERadioStation by Josef Spillner <spillner@kde.org>

*/

class StreamBrowserListView : public KListView {
public:
        StreamBrowserListView( QWidget *parent ) : KListView( parent ) {}

        void startDrag();
};

class StreamBrowser : public QWidget {

Q_OBJECT

public:
        StreamBrowser( QWidget *parent=0, const char *name=0 );
        ~StreamBrowser();

signals:
        void signalNewMetaserver(QString uri);
        void signalStations();

public slots:
        void slotActivate(QListViewItem *item);
        void slotConnected();
        void slotRead();
        void slotError(int error);
        void slotTimeout();

        void slotUpdateMetaservers();
        void slotUpdateStations();

private:
        void doconnection(QString query);
        void doupdate(QString query, QString uri);
        void process(QString content);
        void processlocal(QString content);
        void startDrag();

        QString m_host;
        int m_port;
        StreamBrowserListView *view;
        QSocket *sock;
        QString m_query;
        QString m_metaquery;
        QString m_curquery;
        QString m_update;
        QStringList metaservers;
        int m_synchronized;
};

#endif
