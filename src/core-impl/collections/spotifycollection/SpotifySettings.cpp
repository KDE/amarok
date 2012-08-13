/****************************************************************************************
 * Copyright (c) 2012 Ryan Feng <odayfans@gmail.com>                                    *
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
#define DEBUG_PREFIX "SpotifySettings"
#include <QtGlobal>
#include <QFile>
#include <KLocale>
#include <KMessageBox>
#include <stdlib.h>
#include "core/support/Debug.h"
#include "SpotifySettings.h"
#include "support/Controller.h"
#include "ui_SpotifyConfigWidget.h"


SpotifySettings::SpotifySettings( QWidget* parent, const QVariantList& args )
: KDialog( parent )
, m_downloadReply( 0 )
{
    DEBUG_BLOCK

    Q_UNUSED( args )

    debug() << "Creating Spotify settings object...";

    setCaption( i18n( "Spotify configuration" ) );
    setButtons( KDialog::Ok | KDialog::Cancel | KDialog::Apply | KDialog::Default );
    enableButtonApply( false );

    QWidget* w = new QWidget;
    m_configWidget = new Ui::SpotifyConfigWidget;
    m_configWidget->setupUi( w );

    m_configWidget->progDownload->hide();

    setMainWidget( w );

    connect( this, SIGNAL( okClicked() ),
            this, SLOT( tryLogin() ) );
    connect( this, SIGNAL( cancelClicked() ),
            this, SLOT( cancel() ) );
    connect( this, SIGNAL( applyClicked() ),
            this, SLOT( tryLogin() ) );
    connect( this, SIGNAL( defaultClicked() ),
            this, SLOT( defaults() ) );

    connect( m_configWidget->lineUsername, SIGNAL( textChanged( const QString& ) ),
            this, SLOT( settingsChanged() ) );
    connect( m_configWidget->linePassword, SIGNAL( textChanged( const QString& ) ),
            this, SLOT( settingsChanged() ) );
    connect( m_configWidget->checkHighQuality, SIGNAL( clicked() ),
            this, SLOT( settingsChanged() ) );

    connect( this, SIGNAL( changed( bool ) ), this, SLOT( enableButtonApply( bool ) ) );

    // Load config from KConfig or KWallet
    load();

    debug() << "Checking Spotify resolver: " << m_config.resolverPath();
    if( !QFile::exists( m_config.resolverPath() ) )
    {
        int res = KMessageBox::questionYesNo( this,
i18n( "Spotify resolver is a program required by Spotify collection,it's missing or not installed correctly on your system, you want to download and install it now?" ),
                    i18n( "Spotify resolver" ) );
        if( res == KMessageBox::Yes )
        {
            tryDownloadResolver();
        }
        else
        {
            // Close config dialog
            cancel();
        }
    }
}

SpotifySettings::~SpotifySettings()
{
    delete m_configWidget;
}

void
SpotifySettings::load()
{
    DEBUG_BLOCK
    m_config.load();
    m_configWidget->lineUsername->setText( m_config.username() );
    m_configWidget->linePassword->setText( m_config.password() );
    m_configWidget->checkHighQuality->setChecked( m_config.highQuality() );
}

void
SpotifySettings::save()
{
    DEBUG_BLOCK
    m_config.setUsername( m_configWidget->lineUsername->text() );
    m_config.setPassword( m_configWidget->linePassword->text() );
    m_config.setHighQuality( m_configWidget->checkHighQuality->isChecked() );
    m_config.save();
}

void
SpotifySettings::defaults()
{
    m_config.reset();
    m_configWidget->lineUsername->setText( m_config.username() );
    m_configWidget->linePassword->setText( m_config.password() );
    m_configWidget->checkHighQuality->setChecked( m_config.highQuality() );
}

void
SpotifySettings::tryLogin()
{
    DEBUG_BLOCK
    Spotify::Controller* controller = The::SpotifyController();

    Q_ASSERT( controller );

    if( !controller->running() )
    {
        controller->setFilePath( m_config.resolverPath() );
        if( !controller->loaded() )
        {
            controller->reload();
        }
        controller->start();
    }
    else
    {
        controller->login( m_configWidget->lineUsername->text(), m_configWidget->linePassword->text() );
    }

    save();
}

void
SpotifySettings::tryDownloadResolver()
{
    DEBUG_BLOCK

    debug() << "Trying to download: " << m_config.resolverDownloadUrl();

    NetworkAccessManagerProxy* manager = The::networkAccessManager();
    QNetworkRequest request( m_config.resolverDownloadUrl() );
    QNetworkReply* reply = manager->get( request );
    m_downloadReply = reply;

    connect( reply, SIGNAL( error( QNetworkReply::NetworkError ) ),
            this, SLOT( slotDownloadError( QNetworkReply::NetworkError ) ) );
    connect( reply, SIGNAL( downloadProgress( qint64, qint64 ) ),
            this, SLOT( slotDownloadProgress( qint64, qint64 ) ) );
    connect( reply, SIGNAL( finished() ),
            this, SLOT( slotDownloadFinished() ) );

    // Show progress bar
    m_configWidget->progDownload->setMinimum( 0 );
    m_configWidget->progDownload->setMaximum( 1000 );
    m_configWidget->progDownload->setValue( 0 );
    m_configWidget->progDownload->show();

    // Hide settings
    m_configWidget->frameMain->hide();
    m_configWidget->lblNote->hide();

    enableButtonApply( false );
    enableButtonOk( false );
    enableButton( Default, false );
}

void
SpotifySettings::slotDownloadError( QNetworkReply::NetworkError error )
{
    Q_UNUSED( error )

    KMessageBox::error( this, i18n( "Error occured while downloading Spotify resolver." ));

    cancel();
}

void
SpotifySettings::slotDownloadProgress( qint64 current, qint64 total )
{
    int value = (double)current/total * m_configWidget->progDownload->maximum();

    m_configWidget->progDownload->setValue( value );
}

void
SpotifySettings::slotDownloadFinished()
{
    DEBUG_BLOCK
    debug() << "Download finished.";

    m_configWidget->progDownload->hide();

    QFile file( m_config.resolverPath() );
    if( !file.open( QIODevice::WriteOnly ) )
    {
        KMessageBox::error( this, i18n( "Failed to open file '%1' to write." ).arg( m_config.resolverPath() ) );
    }
    else
    {
        while( m_downloadReply->bytesAvailable() )
        {
            file.write( m_downloadReply->read( 102400 ) );
        }

        file.close();

#if defined(Q_WS_X11) || defined(Q_WS_MAC)
        // Grant execution permission for *nix systems
        QString cmd = QString("chmod +x '%1'").arg(m_config.resolverPath());
        system(qPrintable(cmd));
#endif
    }

    // Restore
    m_configWidget->progDownload->hide();
    m_configWidget->frameMain->show();
    m_configWidget->lblNote->show();

    enableButtonOk( true );
    enableButton( Default, true );
}

void
SpotifySettings::settingsChanged()
{
    emit changed( true );
}

void
SpotifySettings::cancel()
{
    close();

    if( m_downloadReply )
        m_downloadReply->deleteLater();

    deleteLater();
}

#include "SpotifySettings.moc"
