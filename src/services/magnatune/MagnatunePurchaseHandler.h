/****************************************************************************************
 * Copyright (c) 2006,2007 Nikolaj Hald Nielsen <nhn@kde.org>                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef MAGNATUNEPURCHASEHANDLER_H
#define MAGNATUNEPURCHASEHANDLER_H

#include <QObject>
#include <kio/job.h>
#include <kio/jobclasses.h>

#include "MagnatuneAlbumDownloader.h"
#include "MagnatuneDownloadDialog.h"
#include "MagnatunePurchaseDialog.h"
#include "MagnatuneMeta.h"


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
   void purchaseAlbum( Meta::MagnatuneAlbum * album );

signals:

    void purchaseCompleted( bool success );

private:
    KIO::TransferJob * m_resultDownloadJob;


    //need a parent to pass to any dialogs we spawn
    QWidget * m_parent;
    MagnatunePurchaseDialog * m_purchaseDialog;
    MagnatuneDownloadDialog * m_downloadDialog;
    MagnatuneAlbumDownloader * m_albumDownloader;
    Meta::MagnatuneAlbum * m_currentAlbum;
    QString m_currentAlbumCoverName;
    bool m_giftCardPurchase;
    bool m_membershipDownload;

    bool parseDownloadXml( const QString &xml );
    void membershipDownload( int membershipType, const QString &username, const QString &password );

    /**
     * This function saves the xml download info received from Magnatune.com after a
     * successful payment. This information can be used to later redownload a lost album,
     * or acquire an album in a different file format. Note that no personal information
     * or credit card number is stored. The information is saved to the amarok config
     * directory in the sub folder magnatune.com/purchases. The name of each info file
     * is genereated from the artist and album names.
     * @param infoXml The info to store.
     */
    void saveDownloadInfo(const QString &infoXml);


protected slots:

    void showPurchaseDialog( const QString &coverTempLocation );
    void xmlDownloadComplete( KJob* downLoadJob );
    void albumDownloadComplete(bool success);
    void albumPurchaseCancelled();

public slots:

    void processPayment( const QString &ccNumber, const QString &expYear, const QString &expMonth, const QString &name, const QString &email, const QString &albumCode, int amount );

    void processGiftCardPayment( const QString &giftCardCode, const QString &name, const QString &email, const QString &albumCode, int amount );



};

#endif
