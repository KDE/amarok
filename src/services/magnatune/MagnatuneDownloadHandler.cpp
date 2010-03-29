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

#include "MagnatuneDownloadHandler.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"
#include "MagnatuneDatabaseHandler.h"
#include "MagnatuneConfig.h"
#include "statusbar/StatusBar.h"

#include <KMessageBox>
#include <ktempdir.h>

#include <QDir>
#include <QFile>
#include <QTextStream>

using namespace Meta;

MagnatuneDownloadHandler::MagnatuneDownloadHandler()
        : QObject()
        , m_downloadDialog( 0 )
        , m_albumDownloader( 0 )
        , m_currentAlbum( 0 )
        , m_membershipDownload( false )
{
}


MagnatuneDownloadHandler::~MagnatuneDownloadHandler()
{
    delete m_downloadDialog;
    delete m_albumDownloader;
}


void MagnatuneDownloadHandler::downloadAlbum( MagnatuneAlbum * album )
{
    DEBUG_BLOCK
    m_currentAlbum = album;

    //do we have a membership that allows free downloads?

    MagnatuneConfig config;

    if ( config.isMember() && config.membershipType() == MagnatuneConfig::DOWNLOAD ) {
        debug() << "membership download...";
        membershipDownload( config.membershipType(), config.username(), config.password() );
    }
}



void MagnatuneDownloadHandler::membershipDownload( int membershipType, const QString &username, const QString &password )
{
    DEBUG_BLOCK

    QString type;
    if( membershipType == MagnatuneConfig::STREAM )
        type = "stream";
    else
         type = "download";
    
    QString purchaseURL = "http://" + username + ":" + password + "@" + type + ".magnatune.com/buy/membership_free_dl_xml?sku=" + m_currentAlbum->albumCode() + "&id=amarok";

    m_membershipDownload = true;

    m_resultDownloadJob = KIO::storedGet( KUrl( purchaseURL ), KIO::NoReload, KIO::HideProgressInfo );
    The::statusBar()->newProgressOperation( m_resultDownloadJob, i18n( "Processing download" ) );
    connect( m_resultDownloadJob, SIGNAL( result( KJob* ) ), SLOT( xmlDownloadComplete( KJob* ) ) );

    
}

void MagnatuneDownloadHandler::xmlDownloadComplete( KJob * downloadJob )
{

    debug() << "xml download complete";

    if ( !downloadJob->error() == 0 )
    {
        //TODO: error handling here
        debug() << "Job error... " << downloadJob->error();
        return ;
    }
    if ( downloadJob != m_resultDownloadJob ) {
        debug() << "Wrong job...";
        return ; //not the right job, so let's ignore it
    }

    KIO::StoredTransferJob* const storedJob = static_cast<KIO::StoredTransferJob*>( downloadJob );
    QString resultXml = QString( storedJob->data() );

    debug() << endl << endl << "result: " << resultXml;


    if ( m_albumDownloader == 0 )
    {
        m_albumDownloader = new MagnatuneAlbumDownloader();
        connect( m_albumDownloader, SIGNAL( downloadComplete( bool ) ), this, SLOT( albumDownloadComplete( bool ) ) );
    }

    if ( m_downloadDialog == 0 )
    {
        m_downloadDialog = new MagnatuneDownloadDialog( m_parent );
        m_downloadDialog->setModal( true );
        connect( m_downloadDialog, SIGNAL( downloadAlbum( MagnatuneDownloadInfo ) ), m_albumDownloader, SLOT( downloadAlbum( MagnatuneDownloadInfo ) ) );
        //connect( m_downloadDialog, SIGNAL( rejected () ), this, SLOT( albumPurchaseCancelled() ) );

    }


    MagnatuneDownloadInfo downloadInfo;
    if ( downloadInfo.initFromString( resultXml, m_membershipDownload ) )
    {

        downloadInfo.setAlbumCode( m_currentAlbum->albumCode() );
        downloadInfo.setCoverUrl( m_currentAlbum->coverUrl() );
        downloadInfo.setAlbumName( m_currentAlbum->prettyName() );
        downloadInfo.setArtistName( m_currentAlbum->albumArtist()->prettyName() );
        
        if ( m_membershipDownload ) {
            MagnatuneConfig config;
            downloadInfo.setMembershipInfo( config.username(), config.password() );
        } else {
            saveDownloadInfo( resultXml );
        }
        
        m_downloadDialog->setDownloadInfo( downloadInfo );
        //m_purchaseDialog->close();

        
        m_downloadDialog->show();
    } else {
        
        KMessageBox::information( m_parent, i18n( "There seems to be an error in the supplied membership information. Please correct this and try again."),i18n("Could not process download") );
    }
}


void MagnatuneDownloadHandler::setParent( QWidget * parent )
{
    m_parent = parent;

}

void MagnatuneDownloadHandler::saveDownloadInfo( const QString &infoXml )
{

    MagnatuneDatabaseHandler dbHandler;

    QDir purchaseDir( Amarok::saveLocation( "magnatune.com/purchases/" ) );

    debug() << "magnatune save location" << purchaseDir.absolutePath();

    //if directory does not exist, create it
    if ( ! purchaseDir.exists () )
    {
        purchaseDir.mkdir( "." );
    }

    QString fileName = m_currentAlbum->albumArtist()->name() + " - " + m_currentAlbum->name();

    QFile file( purchaseDir.absolutePath() + '/' + fileName );

    //Skip if file already exists
    if ( file.exists () )
        return ;

    //write info
    if ( file.open( QIODevice::WriteOnly | QIODevice::Text ) )
    {
        QTextStream stream( &file );
        stream << infoXml << "\n";
        file.close();
    }
}

void MagnatuneDownloadHandler::albumDownloadComplete( bool success )
{
    //cleanup time!

    debug() << "MagnatuneDownloadHandler::albumDownloadComplete";

    delete m_downloadDialog;
    m_downloadDialog = 0;

    emit( downloadCompleted( success ) );

}


#include "MagnatuneDownloadHandler.moc"



