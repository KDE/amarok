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

class StreamBrowser : public KListView
{
        Q_OBJECT

    public:
        StreamBrowser( QWidget *parent=0, const char *name=0 );
        ~StreamBrowser();

        QStringList metaservers(bool writeable);
        void addStation(QString metaserver, QString stream, QString uri,
            QString location, QString speed, QString style, QString type);

    signals:
        void signalNewMetaserver(QString uri);
        void signalStations();

    public slots:
        void slotActivate(QListViewItem *item);
        void slotConnected();
        void slotRead();
        void slotError(int error);
        void slotTimeout();

        void slotShare();
        void slotUpdateMetaservers();
        void slotUpdateStations();

    private:
        void doconnection(QString query);
        void doupdate( QString, QString );
        void process( QString );
        void processlocal( QString );
        void processicecast( QString );
        void startDrag();

        void savecache();
        void loadcache();

        QString m_host;
        int m_port;
        QSocket *sock;
        QString m_query;
        QString m_metaquery;
        QString m_curquery;
        QString m_update;
        QStringList m_metaservers;
        int m_synchronized;
};


#include <kdialogbase.h>

class QLineEdit;
class QComboBox;

class Share : public KDialogBase
{
        Q_OBJECT
        public:
                Share(QWidget *parent = NULL, const char *name = NULL);
                QString value(QString param);
                void setUris(QStringList uris);
                QString uri();

        private:
                QLineEdit *estream, *euri, *espeed, *elocation, *estyle;
                QComboBox *uribox, *typebox;
};

#endif
