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

#define HTML_FILE KGlobal::dirs()->saveLocation( "data", "amarok/" ) + "contextbrowser.html"

UniversalAmarok::UniversalAmarok(KInstance *inst,QObject *parent,QWidget *widgetParent, QString &desktopName, const char* name):
                   KonqSidebarPlugin(inst,parent,widgetParent,desktopName,name)
{
    widget=new QVBox(widgetParent);
    widget->show();
    widget->resize(300,300);
    browser = new KHTMLPart( widget );
    browser->setDNDEnabled( true );
    updateBrowser(HTML_FILE);
    amarokDCOP=new DCOPClient();
    amarokDCOP->attach();
    
    QHBox* buttonsLayout=new QHBox(widget);
    buttonsLayout->setMargin(0);
    buttonsLayout->setSpacing(0);
    KPushButton* b_prev=new KPushButton(buttonsLayout,"prev"); 
    KPushButton* b_play=new KPushButton(buttonsLayout,"play"); 
    KPushButton* b_pause=new KPushButton(buttonsLayout,"pause"); 
    KPushButton* b_stop=new KPushButton(buttonsLayout,"stop"); 
    KPushButton* b_next=new KPushButton(buttonsLayout,"next"); 
    
    b_prev->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum) );
    b_play->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum) );
    b_pause->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum) );
    b_stop->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum) );
    b_next->setSizePolicy(QSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum) );
    
    b_prev->setIconSet( QIconSet ( KGlobal::iconLoader()->loadIcon( "player_start", KIcon::FirstGroup, KIcon::SizeMedium ) ) );
    b_play->setIconSet( QIconSet ( KGlobal::iconLoader()->loadIcon( "player_play", KIcon::FirstGroup, KIcon::SizeMedium ) ) );
    b_pause->setIconSet( QIconSet ( KGlobal::iconLoader()->loadIcon( "player_pause", KIcon::FirstGroup, KIcon::SizeMedium ) ) );
    b_stop->setIconSet( QIconSet ( KGlobal::iconLoader()->loadIcon( "player_stop", KIcon::FirstGroup, KIcon::SizeMedium ) ) );
    b_next->setIconSet( QIconSet ( KGlobal::iconLoader()->loadIcon( "player_end", KIcon::FirstGroup, KIcon::SizeMedium ) ) );
    connect(b_prev,SIGNAL(clicked() ), this, SLOT(sendControl() ) );
    connect(b_play,SIGNAL(clicked() ), this, SLOT(sendControl() ) );
    connect(b_pause,SIGNAL(clicked() ), this, SLOT(sendControl() ) );
    connect(b_stop,SIGNAL(clicked() ), this, SLOT(sendControl() ) );
    connect(b_next,SIGNAL(clicked() ), this, SLOT(sendControl() ) );
    
    fileInfo = new QFileInfo(HTML_FILE);
    QTimer *t = new QTimer( this );
    connect( t, SIGNAL(timeout()), SLOT(updateStatus() ) );
    t->start( 2000, FALSE );
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
        map->insert ("Name", i18n ("Amarok"));
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

void UniversalAmarok::sendControl()
{
    amarokDCOP->send("amarok", "player", QCString(sender()->name() ) + "()", "");
//     KMessageBox(NULL,QString("amarok")+"player"+QCString(sender()->name() ) + "()");
}
