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

#include "SpotifySettingsDialog.h"
#include "ui_SpotifySettingsWidget.h"
#include "SpotifyDownloadDialog.h"
#include "SpotifyCollection.h"

#include "core/support/Debug.h"
#include "support/Controller.h"

#include <KLocale>

#include <QScopedPointer>
#include <QtGlobal>

SpotifySettingsDialog::SpotifySettingsDialog( QWidget *parent )
    : KDialog( parent )
    , m_settingsWidget(new Ui::SpotifySettingsWidget)
{
    debug() << "Checking Spotify resolver: " << Collections::SpotifyCollection::resolverPath();
    if( !QFile::exists( Collections::SpotifyCollection::resolverPath() ) )
    {
        SpotifyDownloadDialog dialog;
        m_config.reset();
        dialog.exec();
    }

    setCaption( i18n( "Spotify configuration" ) );
    setButtons( KDialog::Ok | KDialog::Cancel | KDialog::Apply | KDialog::Default );
    enableButtonApply( false );

    QWidget *w = new QWidget(this);
    m_settingsWidget->setupUi( w );
    m_settingsWidget->messageWidget->hide();
    setMainWidget( w );

    connect( this, SIGNAL( okClicked() ),
            this, SLOT( slotTryLogin() ) );
    connect( this, SIGNAL( cancelClicked() ),
            this, SLOT( slotCancel() ) );
    connect( this, SIGNAL( applyClicked() ),
            this, SLOT( slotTryLogin() ) );
    connect( this, SIGNAL( defaultClicked() ),
            this, SLOT( defaults() ) );

    connect( m_settingsWidget->lineUsername, SIGNAL( textChanged( const QString& ) ),
            this, SLOT( slotSettingsChanged() ) );
    connect( m_settingsWidget->linePassword, SIGNAL( textChanged( const QString& ) ),
            this, SLOT( slotSettingsChanged() ) );
    connect( m_settingsWidget->checkHighQuality, SIGNAL( clicked() ),
            this, SLOT( slotSettingsChanged() ) );

    connect( this, SIGNAL( changed( bool ) ), this, SLOT( enableButtonApply( bool ) ) );

    // Load config from KConfig or KWallet
    load();
}

SpotifySettingsDialog::~SpotifySettingsDialog()
{
    delete m_settingsWidget;
}

void
SpotifySettingsDialog::load()
{
    DEBUG_BLOCK
    m_config.load();
    m_settingsWidget->lineUsername->setText( m_config.username() );
    m_settingsWidget->linePassword->setText( m_config.password() );
    m_settingsWidget->checkHighQuality->setChecked( m_config.highQuality() );
}

void
SpotifySettingsDialog::save()
{
    DEBUG_BLOCK
    m_config.setUsername( m_settingsWidget->lineUsername->text() );
    m_config.setPassword( m_settingsWidget->linePassword->text() );
    m_config.setHighQuality( m_settingsWidget->checkHighQuality->isChecked() );
    m_config.save();
}

void
SpotifySettingsDialog::defaults()
{
    m_config.reset();
    m_settingsWidget->lineUsername->setText( m_config.username() );
    m_settingsWidget->linePassword->setText( m_config.password() );
    m_settingsWidget->checkHighQuality->setChecked( m_config.highQuality() );
}

void
SpotifySettingsDialog::slotTryLogin()
{
    DEBUG_BLOCK
    Spotify::Controller* controller = The::SpotifyController();

    Q_ASSERT( controller );

    if( !controller->running() || !controller->loaded() )
    {
        controller->setFilePath( Collections::SpotifyCollection::resolverPath() );
        controller->start();
    }

    connect(controller, SIGNAL(customMessage(QString,QVariantMap)),
                        SLOT(slotCustomMessage(QString,QVariantMap)));
    connect(controller, SIGNAL(loginSuccess(QString)), SLOT(slotLoginSuccess(QString)));
    connect(controller, SIGNAL(loginFailed(QString)), SLOT(slotLoginFailed(QString)));

    controller->login( m_settingsWidget->lineUsername->text(),
                       m_settingsWidget->linePassword->text(),
                       m_settingsWidget->checkHighQuality->isChecked() );
    save();
}

void SpotifySettingsDialog::slotLoginSuccess(const QString &username)
{
    m_settingsWidget->messageWidget->setText(i18n("%1 logged in").arg(username));
    m_settingsWidget->messageWidget->setMessageType(KMessageWidget::Positive);
    m_settingsWidget->messageWidget->animatedShow();
}

void SpotifySettingsDialog::slotLoginFailed(const QString &message)
{
    //TODO: translate message
    m_settingsWidget->messageWidget->setText(message);
    m_settingsWidget->messageWidget->setMessageType(KMessageWidget::Error);
    m_settingsWidget->messageWidget->animatedShow();
}

void SpotifySettingsDialog::slotCustomMessage(const QString &messageType,
                                              const QVariantMap &map)
{
    debug() << messageType;
    debug() << map;
    //TODO: take message from map and display in m_settingsWidget->messageWidget
    m_settingsWidget->messageWidget->setText("custom message received");
    m_settingsWidget->messageWidget->animatedShow();
}

void
SpotifySettingsDialog::slotSettingsChanged()
{
    emit changed( true );
}

void
SpotifySettingsDialog::slotCancel()
{
    close();
    deleteLater();
}
