//
//
// C++ Implementation: $MODULE$
//
// Description: 
//
//
// Author: Max Howell, (C) 2003
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "streambrowser.h"

/*
 * Based on KDE Radio Station
 * Copyright (C) 2003 Josef Spillner <spillner@kde.org>
 * Published under GNU GPL conditions.
 */

//#include <kstdguiitem.h>
#include <klocale.h>
#include <klistview.h>
#include <kio/netaccess.h>
#include <kprocess.h>
#include <kmessagebox.h>
#include <ktempfile.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kconfig.h>
#include <kapplication.h>

#include <qlayout.h>
#include <qpushbutton.h>
#include <qdom.h>
#include <qpixmap.h>
#include <qlistview.h>
#include <qfile.h>
#include <qregexp.h>
#include <qtimer.h>

StreamBrowser::StreamBrowser( QWidget *parent, const char *name )
      : QWidget( parent, name, Qt::WType_TopLevel )
{
        QVBoxLayout *vbox;
        KConfig *config;
        QString sync;

        QPushButton *button = new QPushButton( "&Fetch Stream Information", this );

        view = new StreamBrowserListView( this );
        view->setAllColumnsShowFocus(true);
        view->addColumn(i18n("Stream"));
        view->addColumn(i18n("Bandwidth"));
        view->addColumn(i18n("Style"));
        view->addColumn(i18n("Location"));
        view->addColumn(i18n("URI"));
        view->addColumn(i18n("Type"));

        vbox = new QVBoxLayout(this, 5);
        vbox->add(button);
        vbox->add(view);


        setCaption(i18n("StreamBrowser (drag streams to playlist)"));
        resize(600, 300);

        m_query = "<?xml version=\"1.0\"><query class=\"metasound\" type=\"connection\">0.1</query>\n";
        m_metaquery = "<?xml version=\"1.0\"><query class=\"metasound\" type=\"meta\">0.1</query>\n";

        m_update = "<?xml version=\"1.0\"?>";
        m_update += "<update class=\"metasound\" type=\"connection\" username=\"%1\" password=\"%2\">";
        m_update += "<option name=\"mode\">add</option><option name=\"version\">0.1</option>";
        m_update += "<option name=\"stream\">%3</option>";
        m_update += "<option name=\"uri\">%4</option>";
        m_update += "<option name=\"location\">%5</option>";
        m_update += "<option name=\"speed\">%6</option>";
        m_update += "<option name=\"style\">%7</option>";
        m_update += "<option name=\"type\">%8</option>";
        m_update += "</update>\n";

        //connect(view, SIGNAL(executed(QListViewItem *)), SLOT(slotActivate(QListViewItem *)));

        config = kapp->config();
        config->setGroup("meta");
        metaservers = config->readListEntry("metaservers");

//        config->setGroup("settings");
//        sync = config->readEntry("synchronization", "startup");

        view->setDragEnabled( true );

        connect( button, SIGNAL( clicked() ), this, SLOT( slotUpdateStations() ) );
        connect( button, SIGNAL( clicked() ), button, SLOT( hide() ) );
}

StreamBrowser::~StreamBrowser()
{}


#include <kurldrag.h>

void StreamBrowserListView::startDrag()
{
    QListViewItem *item = currentItem();

    if( item )
    {
      KURLDrag *d = new KURLDrag( KURL::List( KURL( item->text(4) ) ), this, "DragObject" );

      kdDebug() << item->text(4) << endl;
      //QDragObject *d = new QTextDrag( myHighlightedText(), this );
      d->dragCopy();
    }
}

void StreamBrowser::slotUpdateMetaservers()
{
        doconnection(m_metaquery);
}


void StreamBrowser::slotUpdateStations()
{
        doconnection(m_query);
}


void StreamBrowser::doconnection(QString query)
{
        QStringList::iterator it;
        bool ret;
        int size;
        QString buf, tmpbuf, tmpname;
        QStringList tmpservers;
        KConfig *config;
        QString alias;
        QTimer timer;

        connect(&timer, SIGNAL(timeout()), SLOT(slotTimeout()));

        m_curquery = query;

        config = new KConfig( "/opt/kde3/share/config/kderadiostationrc" );

        for(QStringList::iterator it = metaservers.begin(); it != metaservers.end(); it++)
        {
                config->setGroup("identifiers");
                alias = config->readEntry((*it));
                if(alias.isNull()) alias = (*it);
                config->setGroup("meta");
                if(config->readNumEntry(alias) != 1) tmpservers += (*it);
        }

        for(it = tmpservers.begin(); it != tmpservers.end(); it++)
        {
                QString tmp = (*it);
                kdDebug() << "META: " << tmp << endl;
                if(tmp.startsWith("ggzmeta://"))
                {
                        KURL url(tmp);
                        m_host = url.host();
                        m_port = url.port();
                        if(!m_port) m_port = 15689;
                        kdDebug() << "metaserver: '" << m_host << "' '" << m_port << "'" << endl;

                        m_synchronized = 0;

                        sock = new QSocket();
                        connect(sock, SIGNAL(connected()), SLOT(slotConnected()));
                        connect(sock, SIGNAL(readyRead()), SLOT(slotRead()));
                        connect(sock, SIGNAL(error(int)), SLOT(slotError(int)));
                        sock->connectToHost(m_host, m_port);

                        timer.start(10000, true);
                        while(!m_synchronized)
                                kapp->processEvents();
                        timer.stop();
                }
                else if(query != m_metaquery)
                {
                        KURL uri(tmp);
#if KDE_IS_VERSION(3,1,92)
                        ret = KIO::NetAccess::download(uri, tmpname, this);
#else
                        ret = KIO::NetAccess::download(uri, tmpname);
#endif

                        if(ret)
                        {
                                QFile f(tmpname);
                                f.open(IO_ReadOnly);
                                while((size = f.readLine(tmpbuf, 1024)) != -1)
                                {
                                        buf += tmpbuf;
                                }
                                f.close();

                                QRegExp exp("\\<\\!\\-\\-([^>])*\\>");
                                buf.replace(exp, "");
//??                                buf = QString::fromUtf8(buf);
                                processlocal(buf);
kdDebug() << buf << endl;

                        }
                }
        }
}

void StreamBrowser::doupdate(QString update, QString uri)
{
        m_curquery = update;

        KURL url(uri);

        sock = new QSocket();
        connect(sock, SIGNAL(connected()), SLOT(slotConnected()));
        connect(sock, SIGNAL(readyRead()), SLOT(slotRead()));
//	connect(sock, SIGNAL(error(int)), SLOT(slotError(int)));
        sock->connectToHost(url.host(), url.port());
}

void StreamBrowser::slotConnected()
{
        sock->writeBlock(m_curquery.latin1(), m_curquery.length());
        sock->flush();
}

void StreamBrowser::slotError(int error)
{
        kdDebug() << "error!" << endl;
        m_synchronized = 1;
}

void StreamBrowser::processlocal(QString content)
{
        QDomDocument dom;
        QDomNode node;
        QDomElement element, child;
        QString buf, tmp, value;
        QDomNamedNodeMap map;

        dom.setContent(content);
        node = dom.documentElement().firstChild();

        if(!node.isNull())
        {
                element = dom.documentElement();
        }

        buf = "<resultset referer=\"\">";

        while(!node.isNull())
        {
                element = node.toElement();
                if((element.tagName() == "entrylist") && (element.attribute("class") == "metasound"))
                {
                        child = element.firstChild().toElement();
                        while(!child.isNull())
                        {
                                if(child.tagName() == "connection")
                                {
                                        tmp = "<result preference=\"0\">";
                                        value = child.text();
                                        value.replace("&", "&amp;");
                                        tmp += "<uri>" + value + "</uri>";
                                        map = child.attributes();
                                        for(unsigned int i = 0; i < map.count(); i++)
                                        {
                                                if(map.item(i).nodeName() != "version")
                                                {
                                                        tmp += "<" + map.item(i).nodeName() + ">";
                                                        tmp += map.item(i).nodeValue();
                                                        tmp += "</" + map.item(i).nodeName() + ">";
                                                }
                                        }
                                        tmp += "</result>";

                                        buf += tmp;
                                }

                                child = child.nextSibling().toElement();
                        }
                }
                node = node.nextSibling();
        }

        buf += "</resultset>";

kdDebug() << buf << endl;
        process(buf);
}

void StreamBrowser::process(QString content)
{
        QDomDocument dom;
        QDomNode node;
        QDomElement element, child;
        QString style, stream, speed, uri, location, type;
        QString name;
        QString grouping;
        KListViewItem *tmp;
        QListViewItem *qtmp;
        KStandardDirs d;
        KConfig *config;
        QString response;
        QStringList::iterator it;
        bool has_metaserver = false;
        bool has_stations = false;

        config = kapp->config();
        config->setGroup("settings");
        grouping = config->readEntry("grouping");

        if(grouping == "flat")
                view->setRootIsDecorated(false);
        else
                view->setRootIsDecorated(true);

        dom.setContent(content);
        node = dom.documentElement().firstChild();

        if(!node.isNull())
        {
                element = dom.documentElement();
                kdDebug() << "we're at " << element.tagName() << endl;
                if(element.attribute("referer") == "update")
                {
                        child = element.firstChild().toElement();
                        response = child.text();
                        KMessageBox::information(this, i18n("Server response was: %1").arg(response), i18n("Update"));
                        return;
                }
        }

        while(!node.isNull())
        {
                stream = QString::null;
                speed = QString::null;
                style = QString::null;
                uri = QString::null;
                location = QString::null;
                type = QString::null;
                name = QString::null;

                element = node.toElement();
                child = element.firstChild().toElement();
                while(!child.isNull())
                {
                        if(child.tagName() == "stream") stream = child.text();
                        else if(child.tagName() == "speed") speed = child.text();
                        else if(child.tagName() == "style") style = child.text();
                        else if(child.tagName() == "uri") uri = child.text();
                        else if(child.tagName() == "location") location = child.text();
                        else if(child.tagName() == "type") type = child.text();
                        else if(child.tagName() == "name") name = child.text();
                        child = child.nextSibling().toElement();
                }
                if(uri.startsWith("ggzmeta://"))
                {
                        for(it = metaservers.begin(); it != metaservers.end(); it++)
                                if((*it) == uri) uri = QString::null;
                        if(!uri.isNull())
                        {
                                metaservers << uri;
                                emit signalNewMetaserver(uri);

                                config->setGroup("meta");
                                config->writeEntry("metaservers", metaservers);
                                config->setGroup("identifiers");
                                config->writeEntry(uri, name);
                                config->sync();
                        }
                        has_metaserver = true;
                }
                else
                {
                        if(!has_stations)
                        {
                                view->clear();
                                has_stations = true;
                        }

                        if(grouping == "bandwidth")
                        {
                                qtmp = view->findItem(speed, 0);
                                if(!qtmp) qtmp = new KListViewItem(view, speed);
                                tmp = new KListViewItem(qtmp, stream, speed, style, location, uri, type);
                        }
                        else if(grouping == "style")
                        {
                                qtmp = view->findItem(style, 0);
                                if(!qtmp) qtmp = new KListViewItem(view, style);
                                tmp = new KListViewItem(qtmp, stream, speed, style, location, uri, type);

                        }
                        else
                                tmp = new KListViewItem(view, stream, speed, style, location, uri, type);
                        tmp->setPixmap(0, QPixmap(d.findResource("data", "kderadiostation/kderadio16.png")));
                }
                node = node.nextSibling();
        }

        if(has_metaserver)
        {
                emit signalNewMetaserver(QString::null);
        }
        if(has_stations)
        {
                emit signalStations();
        }

        kdDebug() << "process is ready" << endl;
}

void StreamBrowser::slotRead()
{
        QString rdata, rtmp;
        QCString cs;

        while(sock->bytesAvailable())
        {
                kdDebug() << sock->bytesAvailable() << endl;
/*		rtmp += sock->readLine();*/
                cs.resize(sock->bytesAvailable());
                sock->readBlock(cs.data(), sock->bytesAvailable());
                rtmp += cs;
                sock->waitForMore(100);
        }
//??        rdata = QString::fromUtf8(rtmp);
kdDebug() << rdata << endl;

        rdata.truncate(rdata.length() - 1);

        process(rdata);

        m_synchronized = 1;

        sock->close();
        //delete sock;
}

void StreamBrowser::slotActivate(QListViewItem *item)
{
        bool success;
        bool cache = true;
        QString tmp, normalized;
        KProcess *proc;
        KConfig *config = kapp->config();
        QString output;

        if(item->text(1).isNull()) return;

        success = true;
        if(item->text(5) == "playlist")
        {
                if(cache)
                {
                        //KMD5 md5(item->text(4));
                        //normalized = QString("kderadiostation-") + QString(md5.hexDigest());
                        normalized = QString("kderadiostation-") + item->text(4);
                        normalized.replace("/", "_");
                        tmp = locateLocal("tmp", normalized);
                }
                else
                {
                        KTempFile t(QString::null, ".pls");
                        tmp = t.name();
                }
                if(!cache || !QFile::exists(tmp))
                {
#if KDE_IS_VERSION(3,1,92)
                        if(KIO::NetAccess::download(item->text(4), tmp, this))
#else
                        if(KIO::NetAccess::download(item->text(4), tmp))
#endif
                                success = true;
                        else
                        {
                                KMessageBox::error(this, i18n("The playlist could not be downloaded."), i18n("Failure"));
                                success = false;
                        }
                }
        }
        else if(item->text(5) == "stream")
        {
                tmp = item->text(4).latin1();
                success = true;
        }
        else
        {
                KMessageBox::error(this, i18n("Unknown type of audio stream."), i18n("Failure"));
                success = false;
        }

        if(success)
        {
                config->setGroup("settings");
                output = config->readEntry("output", "xmms");
                proc = new KProcess();
                *proc << output.latin1() << tmp.latin1();
                proc->start();
        }
}

void StreamBrowser::slotTimeout()
{
        kdDebug() << "timeout!" << endl;

        m_synchronized = 1;
        sock->close();
        //delete sock;
}



