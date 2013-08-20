/****************************************************************************************
 * Copyright (c) 2012 Sven Krohlas <sven@asbest-online.de>                              *
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

#include "AmzDownloader.h"

#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>

AmzDownloader::AmzDownloader( QWidget* parent ) :
    QDialog( parent ),
    ui( new Ui::AmzDownloader )
{
    m_downloadDir = QDesktopServices::storageLocation( QDesktopServices::MusicLocation );

    // parse arguments
    QStringList arguments = QApplication::arguments();

    if( arguments.contains( "--amz" ) )
    {
        int position = arguments.indexOf( "--amz" );

        while( ( position + 1 < arguments.size() ) )
        {
            if( !arguments.at( position + 1 ).startsWith( "--" ) )
                m_amzList += arguments.at( position + 1 );
            else
                break;

            position++;
        }
    }

    checkAmzList();

    if( arguments.contains( "--output-dir" ) )
    {
        int position = arguments.indexOf( "--output-dir" );

        if( ( position + 1 <= arguments.size() ) && !arguments.at( position + 1 ).startsWith( "--" ) && m_downloadDir.exists( arguments.at( position + 1 ) ) )
            m_downloadDir = arguments.at( position + 1 );
    }

    ui->setupUi( this );
    ui->downloadDirectoryEdit->setText( m_downloadDir.absolutePath() );

    if( !m_amzList.isEmpty() )
        ui->amzFileEdit->setText( m_amzList.at( 0 ) );

    connect( ui->quitButton, SIGNAL(clicked()), this, SLOT(quitClicked()) );
    connect( ui->selectAmzButton, SIGNAL(clicked()), this, SLOT(selectAmzClicked()) );
    connect( ui->selectDirectoryButton, SIGNAL(clicked()), this, SLOT(selectDirectoryClicked()) );
    connect( ui->startButton, SIGNAL(clicked()), this, SLOT(startClicked()) );
}

AmzDownloader::~AmzDownloader()
{
    delete ui;
}


// public slots

void
AmzDownloader::quitClicked()
{
    QApplication::quit();
}

void
AmzDownloader::selectAmzClicked()
{
    QFileDialog fileDialog( this );
    fileDialog.setAcceptMode( QFileDialog::AcceptOpen );
    fileDialog.setFileMode( QFileDialog::ExistingFile );
    fileDialog.setNameFilter( tr( "Amazon MP3 Purchase Playlist (*.amz)" ) );

    if( !m_amzList.isEmpty() )
        fileDialog.setDirectory( m_amzList.at( 0 ) );

    if( fileDialog.exec() )
    {
        m_amzList = fileDialog.selectedFiles();
        ui->amzFileEdit->setText( m_amzList.at( 0 ) );
    }
}

void
AmzDownloader::selectDirectoryClicked()
{
    QFileDialog fileDialog( this );
    fileDialog.setAcceptMode( QFileDialog::AcceptOpen );
    fileDialog.setFileMode( QFileDialog::Directory );
    fileDialog.setOptions( QFileDialog::ShowDirsOnly );
    fileDialog.setDirectory( m_downloadDir );

    if( fileDialog.exec() )
    {
        m_downloadDir = fileDialog.selectedFiles().at( 0 );
        ui->downloadDirectoryEdit->setText( m_downloadDir.absolutePath() );
    }
}

void
AmzDownloader::startClicked()
{
    QStringList arguments;
    arguments << "-d" << m_downloadDir.absolutePath();
    arguments << m_amzList;

    m_clamzProcess.setProcessChannelMode( QProcess::MergedChannels );
    m_clamzProcess.start( "clamz", arguments );

    connect( &m_clamzProcess, SIGNAL(error()), this, SLOT(clamzError()) );
    connect( &m_clamzProcess, SIGNAL(readyRead()), this, SLOT(clamzOutputAvailable()) );
    connect( &m_clamzProcess, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(clamzFinished(int,QProcess::ExitStatus)) );

    ui->selectAmzButton->setDisabled( true );
    ui->startButton->setDisabled( true );
    ui->selectDirectoryButton->setDisabled( true );
}


// private

void
AmzDownloader:: checkAmzList()
{
    m_amzList.removeDuplicates();
    QStringList errorList, tmpList = m_amzList;

    for( int i = 0; i < m_amzList.size(); i++ )
    {
        if( !QFile::exists( m_amzList.at( i ) ) )
        {
            errorList << m_amzList.at( i );
            tmpList.removeOne( m_amzList.at( i ) );
        }
    }

    m_amzList = tmpList;

    if( !errorList.isEmpty() )
    {
        QMessageBox errorBox;
        QString errorMsg;

        errorMsg = tr( "The following files do not exist: " );

        for( int i = 0; i < errorList.size(); i++ )
            errorMsg += '\n' + errorList.at( i );

        errorBox.warning( this, tr( "Some files are not available" ), errorMsg );
    }
}


// private slots

void
AmzDownloader::clamzError()
{
    if( m_clamzProcess.error() == QProcess::FailedToStart )
        QMessageBox::warning( this, tr( "Unable to start clamz" ), tr( "Unable to start clamz. has it been installed correctly?" ) );
    else
        QMessageBox::warning( this, tr( "Unexpected error" ), tr( "Unexpected error downloading with clamz. This should not have happened..." ) );
}

void
AmzDownloader::clamzFinished( int exitCode, QProcess::ExitStatus exitStatus )
{
    Q_UNUSED( exitStatus );

    if( exitCode == 0 )
        QMessageBox::information( this, tr( "Download finished" ), tr( "Download finished successfully. Have fun listening to your music." ) );
    else
        QMessageBox::warning( this, tr( "Download failed" ), tr( "Please check the progress output for further information." ) );

    ui->selectAmzButton->setDisabled( false );
    ui->startButton->setDisabled( false );
    ui->selectDirectoryButton->setDisabled( false );
}

void
AmzDownloader::clamzOutputAvailable()
{
    ui->progressBrowser->append( m_clamzProcess.readAll() );
}
