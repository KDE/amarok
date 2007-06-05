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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/ 


#include "debug.h"
#include "magnatunedownloaddialog.h"

#include <kfiledialog.h>
#include <kurlrequester.h>

#include <QCheckBox>
#include <QComboBox>
#include <q3textedit.h>


MagnatuneDownloadDialog::MagnatuneDownloadDialog( QWidget *parent, const char *name, bool modal, Qt::WFlags fl )
        : MagnatuneDownloadDialogBase( parent, name, modal, fl )
{
    downloadTargetURLRequester->fileDialog() ->setMode( KFile::Directory );
    m_currentDownloadInfo = 0;

}

MagnatuneDownloadDialog::~MagnatuneDownloadDialog()
{
    delete m_currentDownloadInfo;
}


void MagnatuneDownloadDialog::downloadButtonClicked( )
{

    if (m_currentDownloadInfo == 0) return;

    m_currentDownloadInfo->setFormatSelection(formatComboBox->currentText());

    KUrl unpackLocation = downloadTargetURLRequester->url();
    unpackLocation.adjustPath(KUrl::AddTrailingSlash);
    m_currentDownloadInfo->setUnpackUrl( unpackLocation.directory( KUrl::ObeyTrailingSlash ) );

    emit( downloadAlbum( m_currentDownloadInfo ) );

    close();

}

void MagnatuneDownloadDialog::setDownloadInfo( MagnatuneDownloadInfo * info )
{
    delete m_currentDownloadInfo;

    m_currentDownloadInfo = info;

    DownloadFormatMap formatMap = info->getFormatMap();

    DownloadFormatMap::Iterator it;

    for ( it = formatMap.begin(); it != formatMap.end(); ++it )
    {
        formatComboBox->addItem( it.key() );
    }

    infoEdit->setText( info->getDownloadMessage() );

}

/*$SPECIALIZATION$*/


#include "magnatunedownloaddialog.moc"

