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
#ifndef KONQUERORSIDEBAR_H
#define KONQUERORSIDEBAR_H

#include <khtml_part.h>
#include <konqsidebarplugin.h>
#include <dcopclient.h> 
#include <qslider.h>
#include <qvbox.h>
#include <khtmlview.h>
#include <kurl.h>
/**
@author Marco Gulino
*/

class universalamarokwidget;
class DCOPClient;
class QFileInfo;
class QDateTime;

class amarokWidget : public QVBox
{
Q_OBJECT
public:
    amarokWidget( QWidget * parent = 0, const char * name = 0, WFlags f = 0 );

protected:
    virtual void dragEnterEvent ( QDragEnterEvent * );
    virtual void dropEvent(QDropEvent*);

signals:
    void emitURL( const KURL &);
};

class htmlWidget : public KHTMLPart
{
Q_OBJECT
public:
    htmlWidget();
    ~htmlWidget();
    virtual bool event ( QEvent * );
signals:
    void emitURL( const KURL &);
};

class UniversalAmarok : public KonqSidebarPlugin
{
Q_OBJECT
public:
    UniversalAmarok(KInstance *inst,QObject *parent,QWidget *widgetParent, QString &desktopName, const char* name=0);

    ~UniversalAmarok();

   virtual QWidget *getWidget(){return (QWidget*)widget;}
   virtual void *provides(const QString &) {return 0;}
   virtual void handleURL(const KURL& /*url*/) {}
    QString getCurrentPlaying();

private:
   amarokWidget* widget;
   KHTMLPart* browser;
   QString amarokPlaying;
   DCOPClient* amarokDCOP;
   QFileInfo* fileInfo;
   QDateTime fileDT;
   QSlider* vol_slider;
   
public slots:
    void updateBrowser(const QString&);
    void updateStatus();
    void sendPrev() { checkForAmarok(); amarokDCOP->send("amarok", "player", "prev()", ""); }
    void sendPlay() { checkForAmarok(); amarokDCOP->send("amarok", "player", "play()", ""); }
    void sendPause() { checkForAmarok(); amarokDCOP->send("amarok", "player", "pause()", ""); }
    void sendStop() { checkForAmarok(); amarokDCOP->send("amarok", "player", "stop()", ""); }
    void sendNext() { checkForAmarok(); amarokDCOP->send("amarok", "player", "next()", ""); }
    void sendMute() { checkForAmarok(); amarokDCOP->send("amarok", "player", "mute()", ""); }
    void volChanged(int vol);
    void openURLRequest( const KURL & );
    void checkForAmarok();
};

#endif
