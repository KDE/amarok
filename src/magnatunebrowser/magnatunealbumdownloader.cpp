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
#include "magnatunealbumdownloader.h"
#include "statusbar.h"


MagnatuneAlbumDownloader::MagnatuneAlbumDownloader()
{}


MagnatuneAlbumDownloader::~MagnatuneAlbumDownloader()
{}

void MagnatuneAlbumDownloader::downloadAlbum( MagnatuneDownloadInfo * info )
{
    KURL downloadUrl = info->getCompleteDownloadUrl();
    m_currentAlbumFileName = downloadUrl.fileName( false );

    m_currentAlbumUnpackLocation = info->getUnpackLocation();



    debug() << "Download: " << downloadUrl.url() << " to: " << m_currentAlbumUnpackLocation << endl;
    debug() << "Using temporary location: " << m_tempDir.name() + m_currentAlbumFileName << endl;

    m_albumDownloadJob = KIO::file_copy( downloadUrl, KURL( m_tempDir.name() + m_currentAlbumFileName ), -1, true, false, false );

    connect( m_albumDownloadJob, SIGNAL( result( KIO::Job* ) ), SLOT( albumDownloadComplete( KIO::Job* ) ) );

    Amarok::StatusBar::instance() ->newProgressOperation( m_albumDownloadJob )
    .setDescription( i18n( "Downloading album" ) )
    .setAbortSlot( this, SLOT( albumDownloadAborted() ) );
}

void MagnatuneAlbumDownloader::downloadCover( QString albumCoverUrlString, QString fileName )
{
    KURL downloadUrl( albumCoverUrlString );

    debug() << "Download Cover: " << downloadUrl.url() << " to: " << m_tempDir.name() << fileName << endl;

    m_albumDownloadJob = KIO::file_copy( downloadUrl, KURL( m_tempDir.name() + fileName ), -1, true, false, false );

    connect( m_albumDownloadJob, SIGNAL( result( KIO::Job* ) ), SLOT( coverDownloadComplete( KIO::Job* ) ) );

    Amarok::StatusBar::instance() ->newProgressOperation( m_albumDownloadJob )
    .setDescription( i18n( "Downloading album cover" ) )
    .setAbortSlot( this, SLOT( coverDownloadAborted() ) );
}



void MagnatuneAlbumDownloader::albumDownloadComplete( KIO::Job * downloadJob )
{

    debug() << "album download complete" << endl;

    if ( !downloadJob->error() == 0 )
    {
        //TODO: error handling here
        return ;
    }
    if ( downloadJob != m_albumDownloadJob )
        return ; //not the right job, so let's ignore it

    //ok, now we have the .zip file downloaded. All we need is to unpack it to the desired location and add it to the collection.

    QString unzipString = "unzip \""+m_tempDir.name() + m_currentAlbumFileName + "\" -d \"" + m_currentAlbumUnpackLocation + "\" &";

    debug() << "unpacking: " << unzipString << endl;

    system( unzipString.ascii() );

    //delete m_albumDownloadJob; //whoa... this crashes everything... but not instantly... Is this job automatically deleted?

    emit( downloadComplete( true ) );

}

void MagnatuneAlbumDownloader::coverDownloadComplete( KIO::Job * downloadJob )
{
  debug() << "cover download complete" << endl;

    if ( !downloadJob || !downloadJob->error() == 0 )
    {
        //TODO: error handling here
        return ;
    }
    if ( downloadJob != m_albumDownloadJob )
        return ; //not the right job, so let's ignore it

    emit( coverDownloadCompleted(  m_tempDir.name() ) );
}


void MagnatuneAlbumDownloader::albumDownloadAborted( )
{
    Amarok::StatusBar::instance()->endProgressOperation( m_albumDownloadJob );
    m_albumDownloadJob->kill( true );
    delete m_albumDownloadJob;
    m_albumDownloadJob = 0;
    debug() << "Aborted album download" << endl;

    emit( downloadComplete( false ) );

}

void MagnatuneAlbumDownloader::coverDownloadAborted( )
{
    Amarok::StatusBar::instance()->endProgressOperation( m_albumDownloadJob );
    m_albumDownloadJob->kill( true );
    delete m_albumDownloadJob;
    m_albumDownloadJob = 0;
    debug() << "Aborted cover download" << endl;

    emit( coverDownloadComplete( false ) );
}







