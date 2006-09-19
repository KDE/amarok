/*
  Copyright (c) 2006  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef MAGNATUNEPURCHASEHANDLER_H
#define MAGNATUNEPURCHASEHANDLER_H

#include <qobject.h>
#include <kio/job.h>
#include <kio/jobclasses.h>

#include "magnatunetypes.h"
#include "magnatunedownloaddialog.h"
#include "magnatunepurchasedialog.h"


/**
The main class responcible for handelig of purchases from Magnatune.com

@author Nikolaj Hald Nielsen
*/
class MagnatunePurchaseHandler : public QObject
{
Q_OBJECT
public:
    MagnatunePurchaseHandler();
    ~MagnatunePurchaseHandler();
    
   void setParent( QWidget * parent );
   void purchaseAlbum( MagnatuneAlbum * album );

protected:
   
    KIO::TransferJob * m_resultDownloadJob;
    KIO::FileCopyJob * m_albumDownloadJob;
        
    //need a parent to passe to any dialogs we spawn
    QWidget * m_parent;
    MagnatunePurchaseDialog * m_purchaseDialog;
    MagnatuneDownloadDialog * m_downloadDialog;
    
    QString m_currentDlUsername;
    QString m_currentDlPassword;
    
    QString m_currentAlbumUnpackLocation;
    QString m_currentAlbumFileName;
    
    bool parseDownloadXml( QString xml );


protected slots:

    void xmlDownloadComplete( KIO::Job* downLoadJob );
    void albumDownloadComplete( KIO::Job* downLoadJob );
    void albumDownloadAborted();

public slots:

    void processPayment( QString ccNumber, QString expYear, QString expMonth, QString name, QString email, QString albumCode, int amount );
    void downloadAlbum( QString url, QString downloadLocation );
    

};

#endif
