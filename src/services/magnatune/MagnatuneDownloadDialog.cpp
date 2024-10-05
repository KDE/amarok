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

#include "MagnatuneDownloadDialog.h"

#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include <KConfigGroup>
#include <KUrlRequester>


MagnatuneDownloadDialog::MagnatuneDownloadDialog( QWidget *parent, Qt::WindowFlags fl  )
    : QDialog( parent, fl )
{
    setupUi(this);
    downloadTargetURLRequester->setMode( KFile::Directory );
    connect( downloadButton, &QPushButton::clicked, this, &MagnatuneDownloadDialog::downloadButtonClicked );
}

MagnatuneDownloadDialog::~MagnatuneDownloadDialog()
{
}


void MagnatuneDownloadDialog::downloadButtonClicked( )
{

    if ( m_currentDownloadInfo.password().isEmpty() ) return;

    QString format = formatComboBox->currentText();
    QString path = downloadTargetURLRequester->url().url();

    //store to config for next download:
    KConfigGroup config = Amarok::config( QStringLiteral("Service_Magnatune") );
    config.writeEntry( "Download Format", format );
    config.writeEntry( "Download Path", path );
    
    m_currentDownloadInfo.setFormatSelection( format );

    QUrl unpackLocation = downloadTargetURLRequester->url();
    m_currentDownloadInfo.setUnpackUrl( unpackLocation.path() );

    Q_EMIT( downloadAlbum( m_currentDownloadInfo ) );

    close();

}

void MagnatuneDownloadDialog::setDownloadInfo( const MagnatuneDownloadInfo &info )
{

    m_currentDownloadInfo = info;

    DownloadFormatMap formatMap = info.formatMap();

    for (DownloadFormatMap::Iterator it = formatMap.begin(), total = formatMap.end(); it != total; ++it )
    {
        formatComboBox->addItem( it.key() );
    }

    infoEdit->setText( info.downloadMessage() );

    //restore format and path from last time, if any.
    KConfigGroup config = Amarok::config( QStringLiteral("Service_Magnatune") );
    QString format = config.readEntry( "Download Format", QString() );
    QString path = config.readEntry( "Download Path", QString() );

    if ( !format.isEmpty() ) {
        int index = formatComboBox->findText( format );
        if ( index != -1 )
            formatComboBox->setCurrentIndex( index );
    }

    if ( !path.isEmpty() ) {
        downloadTargetURLRequester->setUrl( QUrl::fromLocalFile(path) );
    }

}

/*$SPECIALIZATION$*/



