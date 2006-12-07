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

#include "magnatunealbumdownloader.h"
#include "magnatunedownloaddialog.h"
#include "magnatunepurchasedialog.h"
#include "magnatunetypes.h"


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
   /**
    * Starts a purchase operation
    * @param album The album to purchase
    */
   void purchaseAlbum( const MagnatuneAlbum &album );

signals:

    void purchaseCompleted( bool success );

private:
    KIO::TransferJob * m_resultDownloadJob;


    //need a parent to pass to any dialogs we spawn
    QWidget * m_parent;
    MagnatunePurchaseDialog * m_purchaseDialog;
    MagnatuneDownloadDialog * m_downloadDialog;
    MagnatuneAlbumDownloader * m_albumDownloader;
    MagnatuneAlbum m_currentAlbum;
    QString m_currentAlbumCoverName;

    bool parseDownloadXml( QString xml );

    /**
     * This function saves the xml download info received from Magnatune.com after a
     * successful payment. This information can be used to later redownload a lost album,
     * or acquire an album in a different file format. Note that no personal information
     * or credit card number is stored. The information is saved to the amarok config 
     * directory in the sub folder magnatune.com/purchases. The name of each info file 
     * is genereated from the artist and album names.
     * @param infoXml The info to store.
     */
    void saveDownloadInfo(QString infoXml);


protected slots:

    void showPurchaseDialog( QString coverTempLocation );
    void xmlDownloadComplete( KIO::Job* downLoadJob );
    void albumDownloadComplete(bool success);
    void albumPurchaseCancelled();

public slots:

    void processPayment( QString ccNumber, QString expYear, QString expMonth, QString name, QString email, QString albumCode, int amount );

    

};

#endif
