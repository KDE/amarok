/***************************************************************************
 *   Copyright (c) 2006, 2007                                              *
 *        Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                   *
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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/ 

#include "magnatuneredownloadhandler.h"
#include "magnatuneredownloadhandler.moc"

#include "amarok.h"
#include "debug.h"


#include "qdir.h"
#include "qmessagebox.h"
#include <klocale.h>



MagnatuneRedownloadHandler::MagnatuneRedownloadHandler(QWidget * parent)
{
    m_parent = parent;
    m_redownloadDialog = 0;
    m_downloadDialog = 0;
    m_albumDownloader = 0;
}


MagnatuneRedownloadHandler::~MagnatuneRedownloadHandler()
{
}

void MagnatuneRedownloadHandler::showRedownloadDialog( )
{

     debug() << "Show redownload dialog" << endl;

    QStringList previousDownloads = GetPurchaseList();

    if (previousDownloads.isEmpty()) {

        //No previously purchased track information found. No more to do here...
        QMessageBox::information( m_parent, i18n( "No purchases found!" ) ,
                                  i18n( "No previous purchases have been found. Nothing to redownload..." ) + '\n' );
        return;
    }

    if (m_redownloadDialog == 0) {
        m_redownloadDialog = new MagnatuneRedownloadDialog( m_parent );
        connect( m_redownloadDialog, SIGNAL( redownload( const QString &) ), this, SLOT( redownload( const QString &) ) );
        connect( m_redownloadDialog, SIGNAL(cancelled() ), this, SLOT( selectionDialogCancelled() ));
    }


    m_redownloadDialog->setRedownloadItems( previousDownloads );

    m_redownloadDialog->show();

}

QStringList MagnatuneRedownloadHandler::GetPurchaseList( )
{
   
    debug() << "MagnatuneRedownloadHandler::GetPurchaseList( )" << endl;
    
    QStringList returnList;
    QDir purchaseInfoDir( Amarok::saveLocation( "magnatune.com/purchases/" ) );

    if ( !purchaseInfoDir.exists () ) {
      return returnList;
    }

    purchaseInfoDir.setFilter( QDir::Files);
    purchaseInfoDir.setSorting( QDir::Name );

    const QFileInfoList list = purchaseInfoDir.entryInfoList();
    QFileInfoList::const_iterator it( list.begin() );
    QFileInfo fi;

    while ( it != list.end() ) {
        fi = *it;
        returnList.append( fi.fileName() );
        ++it;
    }
     debug() << "Done parsing previous purchases!" << endl;
    return returnList;

}

void MagnatuneRedownloadHandler::redownload( const QString &storedInfoFileName )
{

    QDir purchaseInfoDir( Amarok::saveLocation( "magnatune.com/purchases/" ) );
    QString absFileName = purchaseInfoDir.absolutePath() + '/' + storedInfoFileName;

   debug() << "Redownload file: " << absFileName << endl;

    if ( m_albumDownloader == 0 )
    {
        m_albumDownloader = new MagnatuneAlbumDownloader();
        connect( m_albumDownloader, SIGNAL( downloadComplete( bool ) ), this, SLOT( albumDownloadComplete( bool ) ) );
    }


    if (m_downloadDialog == 0) {
        m_downloadDialog = new MagnatuneDownloadDialog(m_parent);
         connect( m_downloadDialog, SIGNAL( downloadAlbum( MagnatuneDownloadInfo *  ) ), m_albumDownloader, SLOT( downloadAlbum( MagnatuneDownloadInfo * ) ) );
    }


    MagnatuneDownloadInfo * downloadInfo = new MagnatuneDownloadInfo();
    if ( downloadInfo->initFromFile( absFileName ) )
    {

        debug() << "Showing download dialog" << endl;
        m_downloadDialog->setDownloadInfo( downloadInfo );
        m_downloadDialog->show();
    }
    else
    {

        QMessageBox::information( m_parent, i18n( "Could not re-download album" ),
                                  i18n( "There seems to be a problem with the selected redownload info file." ) + '\n' );

    }

}

void MagnatuneRedownloadHandler::selectionDialogCancelled( )
{
    if (m_redownloadDialog != 0) {
        m_redownloadDialog->hide();
        delete m_redownloadDialog;
        m_redownloadDialog = 0;
    }
}

void MagnatuneRedownloadHandler::albumDownloadComplete( bool success )
{
    Q_UNUSED( success );
    //cleanup time!

    if (m_downloadDialog != 0) {
       delete m_downloadDialog;
       m_downloadDialog = 0;
    }
    if (m_albumDownloader != 0) {
        delete m_albumDownloader;
        m_albumDownloader = 0;
    }

}




