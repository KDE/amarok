/***************************************************************************
 *   Copyright (C) 2004 by Marco Gulino                                    *
 *   marco@Paganini                                                        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "universalamarok.h"

#include <qlabel.h>
#include <kinstance.h>
#include <klocale.h>
#include <qstring.h>
#include <qwidget.h>
#include <khtml_part.h>
#include <kglobal.h>
#include <kstandarddirs.h> 
#include <qlayout.h>
#include <qvbox.h>
#include <qtimer.h>
#include <dcopclient.h> 
#include <kmessagebox.h> 
#include <kpushbutton.h> 
#include <kiconloader.h> 
#include <qdatetime.h> 
#include <qfileinfo.h> 
#include <ktoolbar.h> 
#include <kapplication.h>


#define HTML_FILE KGlobal::dirs()->saveLocation( "data", "amarok/" ) + "contextbrowser.html"

UniversalAmarok::UniversalAmarok(KInstance *inst,QObject *parent,QWidget *widgetParent, QString &desktopName, const char* name):
                   KonqSidebarPlugin(inst,parent,widgetParent,desktopName,name)
{
    widget=new QVBox(widgetParent);
    widget->show();
    widget->resize(380,300);
    browser = new KHTMLPart( widget );
    browser->setDNDEnabled( true );
    updateBrowser(HTML_FILE);
    amarokDCOP=new DCOPClient();
    amarokDCOP->attach();
    KToolBar* toolBar=new KToolBar(widget, "PlayerControls");
    toolBar->insertButton("player_start",0,SIGNAL(clicked() ),this, SLOT(sendPrev() ) );
    toolBar->insertButton("player_play",0,SIGNAL(clicked() ),this, SLOT(sendPlay() ) );
    toolBar->insertButton("player_pause",0,SIGNAL(clicked() ),this, SLOT(sendPause() ) );
    toolBar->insertButton("player_stop",0,SIGNAL(clicked() ),this, SLOT(sendStop() ) );
    toolBar->insertButton("player_end",0,SIGNAL(clicked() ),this, SLOT(sendNext() ) );
    
    fileInfo = new QFileInfo(HTML_FILE);
    QTimer *t = new QTimer( this );
    connect( t, SIGNAL(timeout()), SLOT(updateStatus() ) );
    t->start( 2000, FALSE );
        connect( browser->browserExtension(), SIGNAL( openURLRequest( const KURL &, const KParts::URLArgs & ) ),
             this,                          SLOT( openURLRequest( const KURL & ) ) );
}


UniversalAmarok::~UniversalAmarok()
{
    delete fileInfo;
}


#include "universalamarok.moc"


extern "C"
{
    void* create_konqsidebar_universalamarok(KInstance *instance,QObject *par,QWidget *widp,QString &desktopname,const char *name)
    {
        return new UniversalAmarok(instance,par,widp,desktopname,name);
    }
};

extern "C" 
{
    bool add_konqsidebar_universalamarok(QString* fn, QString* param, QMap<QString,QString> *map) 
        {
        Q_UNUSED(param);
                
        map->insert ("Type", "Link");
        map->insert ("URL", "");
        map->insert ("Icon", "amarok");
        map->insert ("Name", i18n ("amaroK"));
        map->insert ("Open", "true");
        map->insert ("X-KDE-KonqSidebarModule","konqsidebar_universalamarok");
        fn->setLatin1 ("amarok.desktop");
        return true;
    }
};


/*!
    \fn UniversalAmarok::updateBrowser()
 */
void UniversalAmarok::updateBrowser(const QString& file)
{
    browser->begin();
    browser->openURL(file);
    browser->end();
}


/*!
    \fn UniversalAmarok::updateStatus()
 */
void UniversalAmarok::updateStatus()
{
    fileInfo->refresh();
    if( fileInfo->lastModified() != fileDT )
    {
        updateBrowser( HTML_FILE );
        fileDT=fileInfo->lastModified();
    }
}


/*!
    \fn UniversalAmarok::getCurrentPlaying()
 */
QString UniversalAmarok::getCurrentPlaying()
{
    QCString returnType;
    QByteArray returnData;
    QString result;
    if(! amarokDCOP->call("amarok", "player", "nowPlaying()", NULL, returnType, returnData) ) return NULL;
    if(returnType!="QString") return NULL;
    QDataStream dataparsing(returnData, IO_ReadOnly);
    dataparsing >> result;
    return result;
}


/*!
    \fn UniversalAmarok::openURLRequest( const KURL &url )
 */
void UniversalAmarok::openURLRequest( const KURL &url )
{
   checkForAmarok();
   QByteArray data;
   QDataStream arg(data, IO_WriteOnly);
   arg << url;
   amarokDCOP->send("amarok", "player", "playMedia(KURL)", data);
}


/*!
    \fn UniversalAmarok::checkForAmarok()
 */
void UniversalAmarok::checkForAmarok()
{
    if(amarokDCOP->isApplicationRegistered("amarok")) return;
    QString amarokSVC;
    int pid;
    KApplication::kdeinitExecWait("amarok");
}
