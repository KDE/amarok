//
// C++ Interface: magnatunepurchasehandler
//
// Description: 
//
//
// Author: Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//
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

@author Mark Kretschmann
*/
class MagnatunePurchaseHandler : public QObject
{
Q_OBJECT
public:
    MagnatunePurchaseHandler();

    ~MagnatunePurchaseHandler();

   void setParent(QWidget * parent);   

   void purchaseAlbum(MagnatuneAlbum * album );

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

   bool parseDownloadXml(QString xml);


protected slots:

   void xmlDownloadComplete( KIO::Job* downLoadJob);
   void albumDownloadComplete( KIO::Job* downLoadJob);
   void albumDownloadAborted();

public slots:

   void processPayment(QString ccNumber, QString expYear, QString expMonth, QString name, QString email, QString albumCode, int amount);
   void downloadAlbum(QString url, QString downloadLocation);
  

};

#endif
