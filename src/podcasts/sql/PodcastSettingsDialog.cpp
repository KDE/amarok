/****************************************************************************************
 * Copyright (c) 2006-2008 Bart Cerneels <bart.cerneels@kde.org>                        *
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

#include "PodcastSettingsDialog.h"
#include "ui_PodcastSettingsBase.h"

#include "Debug.h"

#include <QApplication>
#include <QClipboard>
#include <QFontMetrics>

PodcastSettingsDialog::PodcastSettingsDialog( Meta::SqlPodcastChannelPtr channel, QWidget* parent )
    : KDialog( parent )
    , m_ps( new Ui::PodcastSettingsBase() )
    , m_channel( channel )
{
    QWidget* main = new QWidget( this );
    m_ps->setupUi( main );
    setMainWidget( main );

    setCaption( i18nc("change options", "Configure %1", m_channel->title() ) );
    setModal( true );
    setButtons( Apply | Cancel | Ok );
    setDefaultButton( Ok );
    showButtonSeparator( true );

    init();
}

void
PodcastSettingsDialog::init()
{
    QString url = m_channel->url().url();
    //TODO:use calculated width based on size of m_ps->m_leftClickLabel
    QString feedUrlText = QWidget::fontMetrics().elidedText(
                    url,
                    Qt::ElideMiddle,
                    275 ); //experimentally determined value. i18n might break this.
    m_ps->m_feedLabel->setText( feedUrlText );
    m_ps->m_feedLabel->setUrl( url );
    connect( m_ps->m_feedLabel, SIGNAL(leftClickedUrl( const QString& )),
                  SLOT(slotFeedUrlClicked( const QString& ) ) );

    m_ps->m_saveLocation->setMode( KFile::Directory | KFile::ExistingOnly );
    m_ps->m_saveLocation->setUrl( m_channel->saveLocation() );

    m_ps->m_autoFetchCheck->setChecked( m_channel->autoScan() );

    if( m_channel->fetchType() == Meta::PodcastChannel::StreamOrDownloadOnDemand )
    {
        m_ps->m_streamRadio->setChecked( true );
        m_ps->m_downloadRadio->setChecked( false );
    }
    else if( m_channel->fetchType() == Meta::PodcastChannel::DownloadWhenAvailable )
    {
        m_ps->m_streamRadio->setChecked( false );
        m_ps->m_downloadRadio->setChecked( true );
    }

    m_ps->m_purgeCheck->setChecked( m_channel->hasPurge() );
    m_ps->m_purgeCountSpinBox->setValue( m_channel->purgeCount() );

    if( !m_channel->hasPurge() )
    {
        m_ps->m_purgeCountSpinBox->setEnabled( false );
        m_ps->m_purgeCountLabel->setEnabled( false );
    }

    m_ps->m_writeTagsCheck->setChecked( false );

    enableButtonApply( false );

    // Connects for modification check
    connect( m_ps->m_saveLocation, SIGNAL(urlSelected(const KUrl &)), SLOT(checkModified()) );
    connect( m_ps->m_autoFetchCheck, SIGNAL(clicked()), SLOT(checkModified()) );
    connect( m_ps->m_streamRadio, SIGNAL(clicked()), SLOT(checkModified()) );
    connect( m_ps->m_downloadRadio, SIGNAL(clicked()), SLOT(checkModified()) );
    connect( m_ps->m_purgeCheck, SIGNAL(clicked()), SLOT(checkModified()) );
    connect( m_ps->m_purgeCountSpinBox, SIGNAL(valueChanged( int )), SLOT(checkModified()) );
    connect( m_ps->m_writeTagsCheck, SIGNAL(clicked()), SLOT(checkModified()) );

    connect( this, SIGNAL(applyClicked()), this ,SLOT(slotApply()) );
    connect( this, SIGNAL(okClicked()), this, SLOT(slotApply()) );
}

void
PodcastSettingsDialog::slotFeedUrlClicked( const QString &url ) //SLOT
{
    //adding url to clipboard for users convenience
    QApplication::clipboard()->setText( url );
}

bool
PodcastSettingsDialog::hasChanged()
{
    bool fetchTypeChanged = true;

    if( ( m_ps->m_streamRadio->isChecked() && m_channel->fetchType() == Meta::PodcastChannel::StreamOrDownloadOnDemand ) ||
        ( m_ps->m_downloadRadio->isChecked() && m_channel->fetchType() == Meta::PodcastChannel::DownloadWhenAvailable ) )
    {
        fetchTypeChanged = false;
    }

    return( m_channel->saveLocation() != m_ps->m_saveLocation->url() ||
            m_channel->autoScan() != m_ps->m_autoFetchCheck->isChecked() ||
            m_channel->hasPurge() != m_ps->m_purgeCheck->isChecked()     ||
            m_channel->purgeCount() != m_ps->m_purgeCountSpinBox->value() ||
            fetchTypeChanged );
}

void
PodcastSettingsDialog::checkModified() //slot
{
    enableButtonApply( hasChanged() );
}

void
PodcastSettingsDialog::slotApply()       //slot
{
    m_channel->setAutoScan( m_ps->m_autoFetchCheck->isChecked() );
    m_channel->setFetchType(
        m_ps->m_downloadRadio->isChecked() ?
        Meta::PodcastChannel::DownloadWhenAvailable :
        Meta::PodcastChannel::StreamOrDownloadOnDemand
    );
    m_channel->setSaveLocation( m_ps->m_saveLocation->url() );

    m_channel->setPurge( m_ps->m_purgeCheck->isChecked() );
    m_channel->setPurgeCount( m_ps->m_purgeCountSpinBox->value() );
    m_ps->m_writeTagsCheck->isChecked();

    enableButtonApply( false );
}

bool PodcastSettingsDialog::configure()
{
    return exec() == QDialog::Accepted;
}

#include "PodcastSettingsDialog.moc"

