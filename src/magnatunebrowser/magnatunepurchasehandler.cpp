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

#include "amarok.h"
#include "debug.h"
#include "magnatunedatabasehandler.h"
#include "magnatunepurchasehandler.h"
#include "statusbar.h"

#include <ktempdir.h>

#include <qdir.h>
#include <qfile.h>
#include <qmessagebox.h>

MagnatunePurchaseHandler::MagnatunePurchaseHandler()
        : QObject()
{

    m_downloadDialog = 0;
    m_purchaseDialog = 0;
    m_albumDownloader = 0;
}


MagnatunePurchaseHandler::~MagnatunePurchaseHandler()
{
    delete m_downloadDialog;
    delete m_purchaseDialog;
    delete m_albumDownloader;
}


void MagnatunePurchaseHandler::purchaseAlbum( const MagnatuneAlbum &album )
{
    m_currentAlbum = album;

    //first lets get the album cover for the album we are about to purchase.
    //Then we can show it on the purchase dialog as well as put it in the
    //same directory as the album.

    QString albumCoverUrlString = album.getCoverURL();

    if ( m_albumDownloader == 0 )
    {
        m_albumDownloader = new MagnatuneAlbumDownloader();
        connect( m_albumDownloader, SIGNAL( coverDownloadCompleted( QString ) ), this, SLOT( showPurchaseDialog( QString ) ) );
    }

    m_currentAlbumCoverName = album.getName() + " - cover.jpg";


    m_albumDownloader->downloadCover( albumCoverUrlString, m_currentAlbumCoverName );

}

void MagnatunePurchaseHandler::showPurchaseDialog(  QString coverTempLocation )
{

    if ( m_albumDownloader != 0 )
    {
        delete m_albumDownloader;
        m_albumDownloader = 0;
    }

    if ( m_purchaseDialog == 0 )
    {
        m_purchaseDialog = new MagnatunePurchaseDialog( m_parent, "PurchaseDialog", true, 0 );

        connect( m_purchaseDialog, SIGNAL( makePurchase( QString, QString, QString, QString, QString, QString, int ) ), this, SLOT( processPayment( QString, QString, QString, QString, QString, QString, int ) ) );
        connect ( m_purchaseDialog, SIGNAL( cancelled() ), this, SLOT( albumPurchaseCancelled() ) );
    }





    if ( m_currentAlbum.getId() != 0 )
    {

        KTempDir tempDir;
        m_purchaseDialog->setAlbum( m_currentAlbum );
        m_purchaseDialog->setCover( coverTempLocation + m_currentAlbumCoverName );
        m_purchaseDialog->show();
    }
}

void MagnatunePurchaseHandler::processPayment( QString ccNumber, QString expYear, QString expMonth, QString name, QString email, QString albumCode, int amount )
{
    QString amountString;
    amountString.setNum( amount, 10 );

    QString purchaseURL = "https://magnatune.com/buy/buy_dl_cc_xml?cc=" + ccNumber + "&mm=" + expMonth + "&yy=" + expYear + "&sku=" + albumCode + "&name=" + name + "&email=" + email + "&id=amarok&amount=" + amountString;

    QString debugPurchaseURL = "https://magnatune.com/buy/buy_dl_cc_xml?cc=**********&mm=**&yy=**&sku=" + albumCode + "&name=" + name + "&email=********&id=amarok&amount=" + amountString;
    debug() << "purchase url : " << debugPurchaseURL << endl;

    m_resultDownloadJob = KIO::storedGet( KURL( purchaseURL ), false, false );
    Amarok::StatusBar::instance() ->newProgressOperation( m_resultDownloadJob ).setDescription( i18n( "Processing Payment" ) );

    connect( m_resultDownloadJob, SIGNAL( result( KIO::Job* ) ), SLOT( xmlDownloadComplete( KIO::Job* ) ) );


}

void MagnatunePurchaseHandler::xmlDownloadComplete( KIO::Job * downloadJob )
{

    debug() << "xml download complete" << endl;

    if ( !downloadJob->error() == 0 )
    {
        //TODO: error handling here
        return ;
    }
    if ( downloadJob != m_resultDownloadJob )
        return ; //not the right job, so let's ignore it

    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( downloadJob );
    QString resultXml = QString( storedJob->data() );

    debug() << endl << endl << "result: " << resultXml << endl << endl;


    if ( m_albumDownloader == 0 )
    {
        m_albumDownloader = new MagnatuneAlbumDownloader();
        connect( m_albumDownloader, SIGNAL( downloadComplete( bool ) ), this, SLOT( albumDownloadComplete( bool ) ) );
    }

    if ( m_downloadDialog == 0 )
    {
        m_downloadDialog = new MagnatuneDownloadDialog( m_parent, "downloaddialog", true, 0 );
        connect( m_downloadDialog, SIGNAL( downloadAlbum( MagnatuneDownloadInfo * ) ), m_albumDownloader, SLOT( downloadAlbum( MagnatuneDownloadInfo * ) ) );

    }




    MagnatuneDownloadInfo * downloadInfo = new MagnatuneDownloadInfo();
    if ( downloadInfo->initFromString( resultXml ) )
    {


        downloadInfo->setAlbumId( m_currentAlbum.getId() );

        saveDownloadInfo( resultXml );
        m_downloadDialog->setDownloadInfo( downloadInfo );
        //m_purchaseDialog->close();
        delete m_purchaseDialog;
        m_purchaseDialog = 0;
        m_downloadDialog->show();
    }
    else
    {

        QMessageBox::information( m_parent, "Could not process payment",
                                  "There seems to be an error in the information entered (check the credit card number), please try again\n" );
        m_purchaseDialog->setEnabled( true );
    }
}


void MagnatunePurchaseHandler::setParent( QWidget * parent )
{
    m_parent = parent;

}

void MagnatunePurchaseHandler::saveDownloadInfo( QString infoXml )
{

    QDir purchaseDir( Amarok::saveLocation( "magnatune.com/purchases/" ) );

    debug() << "magnatune save location" << purchaseDir.absPath() << endl;

    //if directory does not exist, create it
    if ( ! purchaseDir.exists () )
    {
        purchaseDir.mkdir( ".", false );
    }

    //Create file name
    MagnatuneArtist artist = MagnatuneDatabaseHandler::instance() ->getArtistById( m_currentAlbum.getArtistId() );
    QString artistName = artist.getName();
    QString fileName = artistName + " - " + m_currentAlbum.getName();

    QFile file( purchaseDir.absPath() + "/" + fileName );

    //Skip if file already exists
    if ( file.exists () )
        return ;

    //write info
    if ( file.open( IO_WriteOnly ) )
    {
        QTextStream stream( &file );
        stream << infoXml << "\n";
        file.close();
    }
}

void MagnatunePurchaseHandler::albumDownloadComplete( bool success )
{
    //cleanup time!

    debug() << "MagnatunePurchaseHandler::albumDownloadComplete" << endl;

    delete m_downloadDialog;
    m_downloadDialog = 0;

    emit( purchaseCompleted( success ) );

}

void MagnatunePurchaseHandler::albumPurchaseCancelled( )
{
    debug() << "Purchased dialog cancelled, deleting..." << endl;

    delete m_purchaseDialog;
    m_purchaseDialog = 0;


    emit( purchaseCompleted( false ) );
}







#include "magnatunepurchasehandler.moc"



