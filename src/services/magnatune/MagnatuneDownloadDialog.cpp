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

#include "Amarok.h"
#include "Debug.h"

#include <kfiledialog.h>
#include <kurlrequester.h>




MagnatuneDownloadDialog::MagnatuneDownloadDialog( QWidget *parent, Qt::WFlags fl )
    : QDialog( parent, fl )
{
    setupUi(this);
    downloadTargetURLRequester->fileDialog()->setMode( KFile::Directory );

}

MagnatuneDownloadDialog::~MagnatuneDownloadDialog()
{
}


void MagnatuneDownloadDialog::downloadButtonClicked( )
{

    if ( m_currentDownloadInfo.password().isEmpty() ) return;

    QString format = formatComboBox->currentText();
    QString path = downloadTargetURLRequester->url().url();;

    //store to config for next download:
    KConfigGroup config = Amarok::config( "Service_Magnatune" );
    config.writeEntry( "Download Format", format );
    config.writeEntry( "Download Path", path );
    
    m_currentDownloadInfo.setFormatSelection( format );

    KUrl unpackLocation = downloadTargetURLRequester->url();
    unpackLocation.adjustPath( KUrl::AddTrailingSlash );
    m_currentDownloadInfo.setUnpackUrl( unpackLocation.directory( KUrl::ObeyTrailingSlash ) );

    emit( downloadAlbum( m_currentDownloadInfo ) );

    close();

}

void MagnatuneDownloadDialog::setDownloadInfo( MagnatuneDownloadInfo info )
{

    m_currentDownloadInfo = info;

    DownloadFormatMap formatMap = info.formatMap();

    DownloadFormatMap::Iterator it;

    for ( it = formatMap.begin(); it != formatMap.end(); ++it )
    {
        formatComboBox->addItem( it.key() );
    }

    infoEdit->setText( info.downloadMessage() );

    //restore format and path from last time, if any.
    KConfigGroup config = Amarok::config("Service_Magnatune");
    QString format = config.readEntry( "Download Format", QString() );
    QString path = config.readEntry( "Download Path", QString() );

    if ( !format.isEmpty() ) {
        int index = formatComboBox->findText( format );
        if ( index != -1 )
            formatComboBox->setCurrentIndex( index );
    }

    if ( !path.isEmpty() ) {
        downloadTargetURLRequester->setUrl( KUrl(path) );
    }

}

/*$SPECIALIZATION$*/


#include "MagnatuneDownloadDialog.moc"

