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
#include <kpushbutton.h> //ctor

#include <qlayout.h>
#include <qdom.h>
#include <qpixmap.h>
#include <qlistview.h>
#include <qfile.h>
#include <qregexp.h>
#include <qtimer.h>

#include <ctime>  //::time()

StreamBrowser::StreamBrowser( QWidget *parent, const char *name )
        : KListView( parent, name )
{
    KConfig *config;
    QString sync;

    setAllColumnsShowFocus(true);
    addColumn(i18n("Stream"));
    addColumn(i18n("Bandwidth"));
    addColumn(i18n("Style"));
    addColumn(i18n("Location"));
    addColumn(i18n("URI"));
    addColumn(i18n("Type"));

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

    config = kapp->config();
    config->setGroup("meta");
    m_metaservers = config->readListEntry("metaservers");

    //        config->setGroup("settings");
    //        sync = config->readEntry("synchronization", "startup");

    setDragEnabled( true );

    KPushButton *share = new KPushButton( KGuiItem( i18n( "Share..." ), "2uparrow" ), parent );
    connect( share, SIGNAL( clicked() ), SLOT( slotShare() ) );
}


StreamBrowser::~StreamBrowser()
{}


void StreamBrowser::slotShare()
{
        int ret;
        QString stream, uri, location, speed, style, type;
        QString alias;
        QStringList serverList = metaservers(true);

        if( serverList.isEmpty() )
        {
                KMessageBox::sorry(this,
                        i18n("No serverList currently accept contributions. "
                                "Try to update your server list."),
                        i18n("Share"));
                return;
        }

        Share s(this);
        s.setUris(serverList);
        ret = s.exec();

        if(ret == QDialog::Accepted)
        {
                uri = s.value("uri");
                location = s.value("location");
                stream = s.value("stream");
                style = s.value("style");
                speed = s.value("speed");
                type = s.value("type");

                alias = s.uri();

                addStation(alias, stream, uri, location, speed, style, type);
        }
}


QStringList StreamBrowser::metaservers(bool writeable)
{
        QStringList tmp;
        QStringList::iterator it;
        KConfig *config;
        bool w;

        if(writeable)
        {
                config = kapp->config();
                config->setGroup("contributions");
                for(it = m_metaservers.begin(); it != m_metaservers.end(); it++)
                {
                        w = config->readBoolEntry((*it));
                        if(w) tmp << (*it);
                }
                return tmp;
        }
        else return m_metaservers;
}

void StreamBrowser::addStation(QString metaserver, QString stream, QString uri, QString location,
        QString speed, QString style, QString type)
{
        QString username, password;
        QString metauri;
        QString update;
        QStringList::iterator it;
        KConfig *config;
        QString cleanurl;

        username = "";
        password = "";

        update = m_update.arg(username
                ).arg(password
                ).arg(stream
                ).arg(uri
                ).arg(location
                ).arg(speed
                ).arg(style
                ).arg(type);
        update.replace("&", "&amp;");
        kdDebug() << "UPDATE: " << update << endl;

        config = kapp->config();
        config->setGroup("identifiers");

        metauri = metaserver;
        for(it = m_metaservers.begin(); it != m_metaservers.end(); it++)
        {
                cleanurl = (*it);
                if(config->readEntry(cleanurl.replace("=", "%3d")) == metaserver) metauri = (*it);
        }

        doupdate(update, metauri);
}

//NEW STUFF ABOVE ME

#include <kurldrag.h>

void StreamBrowser::startDrag()
{
    QListViewItem *item = currentItem();

    if( item )
    {
        KURLDrag *d = new KURLDrag( KURL::List( KURL( item->text(4) ) ), this, "DragObject" );
        d->dragCopy();
    }
}

#include <kcursor.h>

void StreamBrowser::slotUpdateMetaservers()
{
    doconnection(m_metaquery);
}


void StreamBrowser::slotUpdateStations()
{
    QApplication::setOverrideCursor( KCursor::waitCursor() );
    doconnection(m_query);
    QApplication::restoreOverrideCursor();
}


void StreamBrowser::doconnection(QString query)
{
        QStringList::iterator it;
        bool ret;
        int size;
        QString tmpbuf, tmpname;
        QStringList tmpservers;
        KConfig *config;
        QString alias, cleanurl;
        QTimer timer;
        time_t time;
        int oldtime;

        connect(&timer, SIGNAL(timeout()), SLOT(slotTimeout()));

        m_curquery = query;

        config = kapp->config();

        for(QStringList::iterator it = m_metaservers.begin(); it != m_metaservers.end(); it++)
        {
                config->setGroup("identifiers");
                cleanurl = (*it);
                alias = config->readEntry(cleanurl.replace("=", "%3d"));
                if(alias.isNull()) alias = (*it);
                config->setGroup("meta");
                if(config->readNumEntry(alias) != 1) tmpservers += (*it);
        }

//        if(query != m_metaquery)
//                clear(); // FIXME: add/remove protocol

        for(it = tmpservers.begin(); it != tmpservers.end(); it++)
        {
                QString buf;
                QString tmp = (*it);
                kdDebug() << "Synchronize resource: " << tmp << endl;

                if(tmp.startsWith("ggzmeta://"))
                {
                        KURL url(tmp);
                        m_host = url.host();
                        m_port = url.port();
                        if(!m_port) m_port = 15689;

                        config = kapp->config();
                        config->setGroup("deltas");
                        oldtime = config->readNumEntry(tmp);
                        if(query == m_query)
                                m_curquery = query.arg(oldtime);
                        else
                                m_curquery = query;
                        time = ::time(NULL);
                        config->writeEntry(tmp, time);
                        config->sync();

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
                        ret = KIO::NetAccess::download( uri, tmpname, this );
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
                                //buf = QString::fromUtf8(buf);

                                QRegExp icecast("\\<entry_list\\>");
                                QRegExp ggzmeta("\\<resultset referer=\"([^\"])*\"\\>");
                                QRegExp metaservlocal("\\<metaserver\\>");

                                if(icecast.search(buf, 0, QRegExp::CaretAtZero) >= 0)
                                {
                                        kdDebug() << "## detected format: icascast" << endl;
                                        processicecast(buf);
                                }
                                else if(ggzmeta.search(buf, 0, QRegExp::CaretAtZero) >= 0)
                                {
                                        kdDebug() << "## detected format: ggzmeta" << endl;
                                        kdDebug() << "## shouldn't happen, not handled yet" << endl;
                                }
                                else if(metaservlocal.search(buf, 0, QRegExp::CaretAtZero) >= 0)
                                {
                                        kdDebug() << "## detected format: metaservlocal" << endl;
                                        processlocal(buf);
                                }
                                else kdDebug() << "## format not recognized" << endl;
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
    //        connect(sock, SIGNAL(error(int)), SLOT(slotError(int)));
    sock->connectToHost(url.host(), url.port());
}

void StreamBrowser::slotConnected()
{
    sock->writeBlock(m_curquery.latin1(), m_curquery.length());
    sock->flush();
}

void StreamBrowser::slotError( int /*error*/ )
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
                    value.replace("&amp", "&;");
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
    QString grouping, cache;
    KListViewItem *tmp;
    QListViewItem *qtmp, *qtmp2;
    KStandardDirs d;
    KConfig *config;
    QString response;
    QStringList::iterator it;
    bool has_metaserver = false;
    bool has_stations = false;
    bool consideration;
    bool writeable;
    QString cleanurl;

    config = kapp->config();
    config->setGroup("settings");
    grouping = config->readEntry("grouping", "flat");
    cache = config->readEntry("cache", "yes");

    if(grouping == "flat")
            setRootIsDecorated(false);
    else
            setRootIsDecorated(true);

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
            stream = "???";
            speed = "???";
            style = "???";
            uri = "???";
            location = "???";
            type = "???";

            name = QString::null;
            writeable = false;

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
                    else if(child.tagName() == "writeable")
                    {
                            if(child.text() == "true") writeable = true;
                            else writeable = false;
                    }
                    child = child.nextSibling().toElement();
            }
            if(uri.startsWith("ggzmeta://"))
            {
                    for(it = m_metaservers.begin(); it != m_metaservers.end(); it++)
                            //if((*it) == uri) uri = QString::null;
                            if((*it) == uri) it = m_metaservers.remove(it);
                    if(!uri.isNull())
                    {
                            m_metaservers << uri;
                            emit signalNewMetaserver(uri);

                            config->setGroup("meta");
                            config->writeEntry("m_metaservers", m_metaservers);
                            config->setGroup("identifiers");
                            cleanurl = uri;
                            config->writeEntry(cleanurl.replace("=", "%3d"), name);
                            config->setGroup("contributions");
                            config->writeEntry(uri, writeable);
                            config->sync();
                    }
                    has_metaserver = true;
            }
            else
            {
                    if(!has_stations)
                    {
                            /*clear();*/
                            has_stations = true;
                    }

                    consideration = true;
                    qtmp = firstChild();

                    if(grouping == "bandwidth")
                    {
                            qtmp2 = findItem(speed, 0);
                            if(qtmp2) qtmp = qtmp2->firstChild();
                    }
                    if(grouping == "style")
                    {
                            qtmp2 = findItem(style, 0);
                            if(qtmp2) qtmp = qtmp2->firstChild();
                    }

                    while(qtmp)
                    {
                            if((qtmp->text(0) == stream) && (qtmp->text(1) == speed) && (qtmp->text(2) == style))
                                    if((qtmp->text(3) == location) && (qtmp->text(4) == uri) && (qtmp->text(5)) == type)
                                            consideration = false;
                            qtmp = qtmp->nextSibling();
                    }

                    if(consideration)
                    {
                            if(grouping == "bandwidth")
                            {
                                    qtmp = findItem(speed, 0);
                                    if(!qtmp) qtmp = new KListViewItem( this, speed );
                                    tmp = new KListViewItem(qtmp, stream, speed, style, location, uri, type);
                            }
                            else if(grouping == "style")
                            {
                                    qtmp = findItem(style, 0);
                                    if(!qtmp) qtmp = new KListViewItem( this, style );
                                    tmp = new KListViewItem(qtmp, stream, speed, style, location, uri, type);

                            }
                            else
                                    tmp = new KListViewItem( this, stream, speed, style, location, uri, type );
                            tmp->setPixmap(0, QPixmap(d.findResource("data", "kderadiostation/kderadio16.png")));
                    }
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
            if(cache == "yes")
                    savecache();
    }

    kdDebug() << "process is ready" << endl;
}


void StreamBrowser::processicecast( QString content )
{
    QDomDocument dom;
    QDomNode node;
    QDomElement element, child, streamchild;
    QString buf, tmp, value, key;

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
        if((element.tagName() == "entry") && (!element.attribute("name").isNull()))
        {
            child = element.firstChild().toElement();
            while(!child.isNull())
            {
                if(child.tagName() == "stream")
                {
                    streamchild = child.firstChild().toElement();

                    tmp = "<result preference=\"0\">";
                    while(!streamchild.isNull())
                    {
                        key = streamchild.tagName();
                        value = streamchild.text();
                        if(key == "listen_url")
                        {
                            tmp += "<uri>" + value + "</uri>";
                        }
                        else if(key == "stream_type")
                        {
                            if(value == "Ogg Vorbis")
                                tmp += "<type>stream</type>";
                            else if(value == "MP3 audio")
                                tmp += "<type>stream</type>";
                            else
                                tmp += "<type>unknown</type>";
                        }
                        else if(key == "stream_description")
                        {
                            tmp += "<stream>" + value + "</stream>";
                        }
                        else if(key == "current_song")
                        {
                            // ignore
                        }
                        else if(key == "genre")
                        {
                            tmp += "<style>" + value + "</style>";
                        }
                        else if(key == "audio_info")
                        {
                            tmp += "<speed>" + value + "</speed>";
                        }
                        streamchild = streamchild.nextSibling().toElement();
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

    kdDebug() << "processicecast delegates to process(" << buf << ")" << endl;
    process(buf);
}


void StreamBrowser::slotRead()
{
    QString rdata, rtmp;
    QCString cs;

    while(sock->bytesAvailable())
    {
        kdDebug() << sock->bytesAvailable() << endl;
        /*                rtmp += sock->readLine();*/
        cs.resize(sock->bytesAvailable());
        sock->readBlock(cs.data(), sock->bytesAvailable());
        rtmp += cs;
        sock->waitForMore(100);
    }
    //rdata = QString::fromUtf8(rtmp);
    rdata = rtmp;
    //kdDebug() << rdata << endl;

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
    QString output;
    QTimer timer;

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
                    if(KIO::NetAccess::download(item->text(4), tmp, this ))
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
}

void StreamBrowser::slotTimeout()
{
    kdDebug() << "timeout!" << endl;

    m_synchronized = 1;
    sock->close();
    //delete sock;
}


void StreamBrowser::savecache()
{
    QListViewItem *qtmp;
    QString file;

    file = locateLocal("data", "kderadiostation/cache");
    QFile f(file);
    if(!f.open(IO_WriteOnly)) return;
    QTextStream stream(&f);

    qtmp = firstChild();
    while(qtmp)
    {
            while(qtmp->firstChild())
            {
                    for(int i = 0; i < qtmp->depth(); i++)
                            stream << " ";
                    stream << qtmp->text(0) << "\n";
                    qtmp = qtmp->firstChild();
            }
            //kdDebug() << "<|||> " << qtmp->text(0) << endl;
            for(int i = 0; i < qtmp->depth(); i++)
                    stream << " ";
            for(int i = 0; !qtmp->text(i).isNull(); i++)
                    stream << qtmp->text(i) << ":::";
            stream << "\n";
            if(qtmp->nextSibling()) qtmp = qtmp->nextSibling();
            else
            {
                    qtmp = qtmp->parent();
                    if(qtmp) qtmp = qtmp->nextSibling();
            }
    }

    f.close();
}


void StreamBrowser::loadcache()
{
    QListViewItem *qtmp, *qtmp2;
    QString file, line;
    QStringList list;
    int i, depth;
    KStandardDirs d;

    file = locateLocal("data", "kderadiostation/cache");
    QFile f(file);
    if(!f.open(IO_ReadOnly)) return;
    QTextStream stream(&f);

    qtmp = NULL;

    while(!stream.eof())
    {
            line = stream.readLine();
            depth = 0;
            while(line.startsWith(" "))
            {
                    line = line.right(line.length() - 1);
                    depth++;
            }
            list = QStringList::split(":::", line);

            while((qtmp) && (depth > qtmp->depth() + 1))
                    qtmp = qtmp->firstChild();
            while((qtmp) && (depth < qtmp->depth() + 1))
                    qtmp = qtmp->parent();
            /*if((!qtmp) && (depth > 0))
            {
                    setRootIsDecorated(true);
                    kdDebug() << "yay for " << line << endl;
                    qtmp = firstChild();
                    kdDebug() << "depth: " << qtmp->depth() << endl;
            }*/

            if(qtmp)
            {
                    qtmp2 = new KListViewItem(qtmp);
                    setRootIsDecorated(true);
            }
            else
            {
                    qtmp2 = new KListViewItem( this );
                    qtmp = qtmp2;
                    qtmp2->setPixmap(0, QPixmap(d.findResource("data", "kderadiostation/kderadio16.png")));
            }
            i = 0;
            for(QStringList::iterator it = list.begin(); it != list.end(); it++, i++)
                    qtmp2->setText(i, (*it));
    }

    f.close();
}



#include <qlabel.h>
#include <qlineedit.h>
#include <qcombobox.h>

Share::Share(QWidget *parent, const char *name)
: KDialogBase(Plain, i18n("Share"), Ok | Cancel, Ok, parent, name, true, true)
{
        QWidget *page = plainPage();
        QVBoxLayout *vbox;
        QLabel *lstream, *luri, *lspeed, *llocation, *ltype, *lstyle, *lcategory;

        lstream = new QLabel(i18n("Stream title"), this);
        lstyle = new QLabel(i18n("Style"), this);
        llocation = new QLabel(i18n("Location"), this);
        lspeed = new QLabel(i18n("Bandwidth in kB"), this);
        ltype = new QLabel(i18n("Type"), this);
        luri = new QLabel(i18n("URI"), this);
        lcategory = new QLabel(i18n("Category"), this);

        estream = new QLineEdit(this);
        estyle = new QLineEdit(this);
        elocation = new QLineEdit(this);
        espeed = new QLineEdit(this);
        typebox = new QComboBox(this);
        euri = new QLineEdit(this);
        uribox = new QComboBox(this);

        typebox->insertItem(i18n("Playlist"));
        typebox->insertItem(i18n("Direct stream"));

        vbox = new QVBoxLayout(page, marginHint(), spacingHint());
        vbox->add(lstream);
        vbox->add(estream);
        vbox->add(lstyle);
        vbox->add(estyle);
        vbox->add(llocation);
        vbox->add(elocation);
        vbox->add(lspeed);
        vbox->add(espeed);
        vbox->add(ltype);
        vbox->add(typebox);
        vbox->add(luri);
        vbox->add(euri);
        vbox->add(lcategory);
        vbox->add(uribox);

        estream->setFocus();
}

QString Share::value(QString param)
{
        if(param == "stream") return estream->text();
        else if(param == "uri") return euri->text();
        else if(param == "speed") return espeed->text();
        else if(param == "location") return elocation->text();
        else if(param == "type") return typebox->currentText();
        else if(param == "style") return estyle->text();
        return QString::null;
}

void Share::setUris(QStringList uris)
{
        QStringList::iterator it;
        KConfig *config;
        QString alias;
        QString cleanurl;

        config = kapp->config();
        config->setGroup("identifiers");

        for (it = uris.begin(); it != uris.end(); it++)
        {
                if((*it).startsWith("ggzmeta://"))
                {
                        cleanurl = (*it);
                        alias = config->readEntry(cleanurl.replace("=", "%3d"), (*it));
                        uribox->insertItem(alias);
                }
        }
}

QString Share::uri()
{
        return uribox->currentText();
}

#include "streambrowser.moc"

