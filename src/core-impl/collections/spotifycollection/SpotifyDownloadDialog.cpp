/****************************************************************************************
 * Copyright (c) 2012 Ryan Feng <odayfans@gmail.com>                                    *
 * Copyright (c) 2012 Bart Cerneels <bart.cerneels@kde.org>                             *
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

#include "SpotifyDownloadDialog.h"
#include "ui_SpotifyDownloadDialog.h"

#include "SpotifyCollection.h"

#include "core/support/Debug.h"
#include "network/NetworkAccessManagerProxy.h"
#include "support/Controller.h"

#include <KZip>

#include <QBuffer>
#include <QFile>

SpotifyDownloadDialog::SpotifyDownloadDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::SpotifyDownloadDialog)
  , m_downloadReply( 0 )
{
    m_ui->setupUi(this);
    m_ui->messageWidget->hide();
    connect(m_ui->buttonBox, SIGNAL(accepted()), SLOT(tryDownloadResolver()));
    connect(m_ui->buttonBox, SIGNAL(rejected()), SLOT(reject()));
    if( Collections::SpotifyCollection::supportedPlatformName().isEmpty() )
    {
        m_ui->messageWidget->setText(i18n( "Your platform is not currently "
                                           "supported by Amarok Spotify resolver." ));
        m_ui->messageWidget->animatedShow();
        m_ui->buttonBox->setEnabled(false);
    }
}

SpotifyDownloadDialog::~SpotifyDownloadDialog()
{
    delete m_ui;
}

void
SpotifyDownloadDialog::tryDownloadResolver()
{
    debug() << "Trying to download: " << Collections::SpotifyCollection::resolverDownloadUrl();

    NetworkAccessManagerProxy* manager = The::networkAccessManager();
    QNetworkRequest request( Collections::SpotifyCollection::resolverDownloadUrl() );
    QNetworkReply* reply = manager->get( request );
    m_downloadReply = reply;

    connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
            this, SLOT( slotDownloadError( QNetworkReply::NetworkError ) ) );
    connect( reply, SIGNAL( downloadProgress( qint64, qint64 ) ),
            this, SLOT( slotDownloadProgress( qint64, qint64 ) ) );
    connect( reply, SIGNAL( finished() ),
            this, SLOT( slotDownloadFinished() ) );

    //set-up progress bar
    m_ui->progDownload->setMinimum( 0 );
    m_ui->progDownload->setMaximum( 1000 );
    m_ui->progDownload->setValue( 0 );
}

void
SpotifyDownloadDialog::slotDownloadError( QNetworkReply::NetworkError error )
{
    Q_UNUSED(error);
    QNetworkReply *reply = qobject_cast<QNetworkReply *>(QObject::sender());
    Q_ASSERT(reply);
    m_ui->messageWidget->setText(i18n("There was an error while downloading"
                "the spotify resolver. Error message: %1.").arg(reply->errorString()));
    m_ui->messageWidget->animatedShow();
}

void
SpotifyDownloadDialog::slotDownloadProgress( qint64 current, qint64 total )
{
    int value = (double)current/total * m_ui->progDownload->maximum();

    m_ui->progDownload->setValue( value );
}

void
SpotifyDownloadDialog::slotDownloadFinished()
{
    if( m_downloadReply->error() != QNetworkReply::NoError )
    {
        m_ui->messageWidget->setText(i18n("Downloading is interrupted due to %1")
                                        .arg(m_downloadReply->errorString()));
        m_ui->messageWidget->animatedShow();
        return;
    }

    QByteArray data( m_downloadReply->readAll() );
    QScopedPointer<QBuffer> data_buffer(new QBuffer(&data));

    KZip archive( data_buffer.data() );
    if( !archive.open( QIODevice::ReadOnly ) || !archive.directory() )
    {
        m_ui->messageWidget->setText(i18n("Failed to read data from the downloaded file. "
                                          "Please try again later."));
        m_ui->messageWidget->animatedShow();
        return;
    }

    archive.directory()->copyTo( Collections::SpotifyCollection::resolverDownloadPath() );

    QFile file( Collections::SpotifyCollection::resolverPath() );
    if( !file.exists() )
    {
        //TODO: display error to the user
        m_ui->messageWidget->setText(i18n( "Failed to extract the Spotify resolver to %1 "
                                           "Please check if the path is writeable." )
                                .arg( Collections::SpotifyCollection::resolverDownloadPath() ));
        m_ui->messageWidget->animatedShow();
        return;
    }
    file.setPermissions( file.permissions() | QFile::ExeUser );

    // Notify controller to load the resolver
    Spotify::Controller* controller = The::SpotifyController();
    controller->setFilePath( Collections::SpotifyCollection::resolverPath() );
    controller->reload();

    accept();
}
