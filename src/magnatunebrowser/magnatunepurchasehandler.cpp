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

#include <qdir.h>
#include <qfile.h>
#include <qmessagebox.h>

MagnatunePurchaseHandler::MagnatunePurchaseHandler()
 : QObject()
{

   m_downloadDialog = 0;
   m_purchaseDialog = 0;
   m_currentAlbum = 0;
}


MagnatunePurchaseHandler::~MagnatunePurchaseHandler()
{
}


void MagnatunePurchaseHandler::purchaseAlbum(MagnatuneAlbum * album )
{
    m_currentAlbum = album;

   if (m_purchaseDialog == 0) {
      m_purchaseDialog = new MagnatunePurchaseDialog(m_parent, "PurchaseDialog", true, 0);
   }
    

   connect(m_purchaseDialog, SIGNAL(makePurchase(QString, QString, QString, QString, QString, QString, int )), this, SLOT(processPayment( QString, QString, QString, QString, QString, QString, int)));


   if (album != 0) {
      m_purchaseDialog->setAlbum(album);
      m_purchaseDialog->show();
   }

}

void MagnatunePurchaseHandler::processPayment( QString ccNumber, QString expYear, QString expMonth, QString name, QString email, QString albumCode, int amount)
{
   QString amountString;
   amountString.setNum(amount, 10);

   QString purchaseURL = "https://magnatune.com/buy/buy_dl_cc_xml?cc=" + ccNumber + "&mm=" + expMonth + "&yy="+ expYear + "&sku=" + albumCode + "&name=" + name + "&email=" + email + "&id=nikolaj&amount=" + amountString;

   debug() << "purchase url : " << purchaseURL << endl;	

   m_resultDownloadJob = KIO::storedGet( KURL(purchaseURL), false, false );
   Amarok::StatusBar::instance()->newProgressOperation( m_resultDownloadJob ).setDescription( i18n( "Processing Payment" ) );
   
   connect( m_resultDownloadJob, SIGNAL( result( KIO::Job* ) ), SLOT( xmlDownloadComplete( KIO::Job* ) ) );


}

void MagnatunePurchaseHandler::xmlDownloadComplete( KIO::Job * downloadJob )
{

     debug() << "xml download complete" << endl;

    if ( !downloadJob->error() == 0 )
    {
       //TODO: error handling here
       return;
    }
    if ( downloadJob != m_resultDownloadJob )
        return; //not the right job, so let's ignore it

    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( downloadJob );
    QString resultXml = QString( storedJob->data() );

    debug() << endl << endl << "result: " << resultXml << endl << endl;	

    saveDownloadInfo( resultXml );
 
    
   if (m_downloadDialog == 0) {
      m_downloadDialog = new MagnatuneDownloadDialog(m_parent, "downloaddialog", true, 0);
     
   }

   connect(m_downloadDialog, SIGNAL(downloadAlbum(QString, QString)), this, SLOT(downloadAlbum(QString, QString)));

   //delete m_resultDownloadJob;

   if (parseDownloadXml(resultXml)) {
      
     m_purchaseDialog->close();
     m_downloadDialog->show();
   } else {

       QMessageBox::information( m_parent, "Could not process payment",
      "There seems to be an error in the information entered (check the credit card number), please try again\n");

      //m_purchaseDialog->show();

      
   }
}



bool MagnatunePurchaseHandler::parseDownloadXml( QString xml )
{


   //complete overkill to do a full SAX2 parser for this at the moment... I think....
   

   // lets make sure that this is actually a valid result

   int testIndex = xml.find("<RESULT>");
   if (testIndex == -1) {
      return false;
   };

   int startIndex;
   int endIndex;

   startIndex = xml.find("<DL_USERNAME>", 0, false);
   if (startIndex != -1) {	
      endIndex = xml.find("</DL_USERNAME>", 0, false);
      if (endIndex != -1) {
         startIndex += 13;

         debug() << "found username: " <<  xml.mid(startIndex, endIndex - startIndex) << endl;
         m_currentDlUsername = xml.mid(startIndex, endIndex - startIndex);
      } else {
         return false;
      }
   } else { 
      return false;
   }

   
   startIndex = xml.find("<DL_PASSWORD>", 0, false);
   if (startIndex != -1) {
      endIndex = xml.find("</DL_PASSWORD>", 0, false);
      if (endIndex != -1) {
         startIndex += 13;
         debug() << "found password: " <<  xml.mid(startIndex, endIndex - startIndex) << endl;
         m_currentDlPassword = xml.mid(startIndex, endIndex - startIndex);
      } else {
         return false;
      }
   } else { 
      return false;
   }


   startIndex = xml.find("<URL_WAVZIP>", 0, false);
   if (startIndex != -1) {
      endIndex = xml.find("</URL_WAVZIP>", 0, false);
      if (endIndex != -1) {
         startIndex += 12;
         debug() << "found wav" <<   endl;
         m_downloadDialog->addFormat("Wav" , xml.mid(startIndex, endIndex - startIndex));

      } 
   } 

   startIndex = xml.find("<URL_128KMP3ZIP>", 0, false);
   if (startIndex != -1) {
      endIndex = xml.find("</URL_128KMP3ZIP>", 0, false);
      if (endIndex != -1) {
         startIndex += 16;
         debug() << "found 128k mp3" <<   endl;
         m_downloadDialog->addFormat("128 kbit/s MP3" , xml.mid(startIndex, endIndex - startIndex));

      } 
   } 

   startIndex = xml.find("<URL_OGGZIP>", 0, false);
   if (startIndex != -1) {
      endIndex = xml.find("</URL_OGGZIP>", 0, false);
      if (endIndex != -1) {
         startIndex += 12;
         debug() << "found ogg-vorbis" <<   endl;
         m_downloadDialog->addFormat("Ogg-Vorbis" , xml.mid(startIndex, endIndex - startIndex));

      } 
   } 

   startIndex = xml.find("<URL_VBRZIP>", 0, false);
   if (startIndex != -1) {
      endIndex = xml.find("</URL_VBRZIP>", 0, false);
      if (endIndex != -1) {
         startIndex += 12;
         debug() << "found vbr mp3" <<   endl;
         m_downloadDialog->addFormat("VBR MP3" , xml.mid(startIndex, endIndex - startIndex));

      } 
   } 

   startIndex = xml.find("<URL_FLACZIP>", 0, false);
   if (startIndex != -1) {
      endIndex = xml.find("</URL_FLACZIP>", 0, false);
      if (endIndex != -1) {
         startIndex += 13;
         debug() << "found flac" <<   endl;
         m_downloadDialog->addFormat("FLAC" , xml.mid(startIndex, endIndex - startIndex));

      } 
   } 

   startIndex = xml.find("<DL_MSG>", 0, false);
   if (startIndex != -1) {
      endIndex = xml.find("</DL_MSG>", 0, false);
      if (endIndex != -1) {
         startIndex += 9;
         debug() << "found dl-message" <<   endl;
         m_downloadDialog->setMessage(xml.mid(startIndex, endIndex - startIndex));
      } 
   } 

  return true;

}

void MagnatunePurchaseHandler::setParent( QWidget * parent )
{
   m_parent = parent;

}

void MagnatunePurchaseHandler::downloadAlbum( QString url, QString downloadLocation)
{
   
   KURL downloadUrl(url);

   m_currentAlbumFileName = downloadUrl.fileName(false);

   m_currentAlbumUnpackLocation = downloadLocation;
  
   downloadUrl.setUser(m_currentDlUsername);
   downloadUrl.setPass(m_currentDlPassword);

   debug() << "Download: " << downloadUrl.url() << " to: " << m_currentAlbumUnpackLocation << endl;	

   m_albumDownloadJob = KIO::file_copy(downloadUrl, KURL("/tmp/" + m_currentAlbumFileName), -1, true, false, false);

   connect( m_albumDownloadJob, SIGNAL( result( KIO::Job* ) ), SLOT( albumDownloadComplete( KIO::Job* ) ) );

   Amarok::StatusBar::instance()->newProgressOperation( m_albumDownloadJob )
      .setDescription( i18n( "Downloading album" ))
      .setAbortSlot( this, SLOT(albumDownloadAborted()) );


}




void MagnatunePurchaseHandler::saveDownloadInfo( QString infoXml )
{

    QDir purchaseDir( Amarok::saveLocation( "magnatune.com/purchases/" ) );
   
    debug() << "magnatune save location" << purchaseDir.absPath() << endl;

    //if directory does not exist, create it
    if (! purchaseDir.exists () )  {
        purchaseDir.mkdir( ".", false );
    }

    //Create file name
    MagnatuneArtist artist = MagnatuneDatabaseHandler::instance()->getArtistById( m_currentAlbum->getArtistId() );
    QString artistName = artist.getName();
    QString fileName = artistName + " - " + m_currentAlbum->getName();

    QFile file( purchaseDir.absPath() + "/" + fileName );

    //Skip if file already exists
    if ( file.exists () )
        return;

    //write info
    if ( file.open( IO_WriteOnly ) ) {
        QTextStream stream( &file );
            stream << infoXml << "\n";
        file.close();
    }
}



void MagnatunePurchaseHandler::albumDownloadComplete( KIO::Job * downloadJob )
{

    debug() << "album download complete" << endl;

    if ( !downloadJob->error() == 0 )
    {
       //TODO: error handling here
       return;
    }
    if ( downloadJob != m_albumDownloadJob )
        return; //not the right job, so let's ignore it

    //ok, now we have the .zip file downloaded. All we need is to unpack it to the desired location and add it to the collection.

   QString unzipString = "unzip \"/tmp/" + m_currentAlbumFileName +"\" -d \"" + m_currentAlbumUnpackLocation + "\" &";

   debug() << "unpacking: " << unzipString << endl;

   system(unzipString.ascii());

   //delete m_albumDownloadJob; //whoa... this crashes everything... but not instantly... Is this job automatically deleted?

   //debug() << "Album download job deleted!" << endl;
}



void MagnatunePurchaseHandler::albumDownloadAborted()
{
   Amarok::StatusBar::instance()->endProgressOperation(m_albumDownloadJob);
   m_albumDownloadJob->kill(true);
   delete m_albumDownloadJob;
   debug() << "Aborted album download" << endl;
}

#include "magnatunepurchasehandler.moc"



