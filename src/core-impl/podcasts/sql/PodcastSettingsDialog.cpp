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
#include "PodcastFilenameLayoutConfigDialog.h"

#include "core/support/Debug.h"

#include <QApplication>
#include <QClipboard>
#include <QFontMetrics>
#include <KConfigGroup>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

PodcastSettingsDialog::PodcastSettingsDialog( const Podcasts::SqlPodcastChannelPtr &channel, QWidget* parent )
    : KPageDialog( parent )
    , m_ps( new Ui::PodcastSettingsBase() )
    , m_channel( channel )
{
    QWidget* main = new QWidget( this );
    m_ps->setupUi( main );

    setWindowTitle( i18nc("change options", "Configure %1", m_channel->title() ) );
    setModal( true );
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel|QDialogButtonBox::Apply);
    setButtonBox(buttonBox);

    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);

    okButton->setDefault(true);
    addPage(main, i18n("Settings") );

    init();
}

void
PodcastSettingsDialog::init()
{
    QString url = m_channel->url().url();
    m_ps->m_urlLineEdit->setText( url );

    m_ps->m_saveLocation->setMode( KFile::Directory | KFile::ExistingOnly );
    m_ps->m_saveLocation->setUrl( m_channel->saveLocation() );

    m_ps->m_autoFetchCheck->setChecked( m_channel->autoScan() );

    if( m_channel->fetchType() == Podcasts::PodcastChannel::StreamOrDownloadOnDemand )
    {
        m_ps->m_streamRadio->setChecked( true );
        m_ps->m_downloadRadio->setChecked( false );
    }
    else if( m_channel->fetchType() == Podcasts::PodcastChannel::DownloadWhenAvailable )
    {
        m_ps->m_streamRadio->setChecked( false );
        m_ps->m_downloadRadio->setChecked( true );
    }

    m_ps->m_purgeCheck->setChecked( m_channel->hasPurge() );
    m_ps->m_purgeCountSpinBox->setValue( m_channel->purgeCount() );
    m_ps->m_purgeCountSpinBox->setSuffix( i18np( " Item", " Items", m_ps->m_purgeCountSpinBox->value() ) );

    if( !m_channel->hasPurge() )
    {
        m_ps->m_purgeCountSpinBox->setEnabled( false );
        m_ps->m_purgeCountLabel->setEnabled( false );
    }

    m_ps->m_writeTagsCheck->setChecked( m_channel->writeTags() );

    buttonBox()->button(QDialogButtonBox::Apply)->setEnabled( false );

    // Connects for modification check
    connect( m_ps->m_urlLineEdit, &QLineEdit::textChanged,
             this, &PodcastSettingsDialog::checkModified );
    connect( m_ps->m_saveLocation, &KUrlRequester::textChanged,
             this, &PodcastSettingsDialog::checkModified );
    connect( m_ps->m_autoFetchCheck, &QAbstractButton::clicked, this, &PodcastSettingsDialog::checkModified );
    connect( m_ps->m_streamRadio, &QAbstractButton::clicked, this, &PodcastSettingsDialog::checkModified );
    connect( m_ps->m_downloadRadio, &QAbstractButton::clicked, this, &PodcastSettingsDialog::checkModified );
    connect( m_ps->m_purgeCheck, &QAbstractButton::clicked, this, &PodcastSettingsDialog::checkModified );
    connect( m_ps->m_purgeCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
             this, &PodcastSettingsDialog::checkModified );
    connect( m_ps->m_writeTagsCheck, &QAbstractButton::clicked, this, &PodcastSettingsDialog::checkModified );
    connect( m_ps->m_filenameLayoutConfigWidgetButton, &QAbstractButton::clicked,
             this, &PodcastSettingsDialog::launchFilenameLayoutConfigDialog );

    connect(buttonBox()->button(QDialogButtonBox::Apply), &QAbstractButton::clicked, this, &PodcastSettingsDialog::slotApply );
    connect(buttonBox()->button(QDialogButtonBox::Ok), &QAbstractButton::clicked, this, &PodcastSettingsDialog::slotApply );
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

    if( ( m_ps->m_streamRadio->isChecked() && m_channel->fetchType() == Podcasts::PodcastChannel::StreamOrDownloadOnDemand ) ||
        ( m_ps->m_downloadRadio->isChecked() && m_channel->fetchType() == Podcasts::PodcastChannel::DownloadWhenAvailable ) )
    {
        fetchTypeChanged = false;
    }

    return( m_channel->url() != QUrl::fromUserInput(m_ps->m_urlLineEdit->text()) ||
            m_channel->saveLocation() != m_ps->m_saveLocation->url() ||
            m_channel->autoScan() != m_ps->m_autoFetchCheck->isChecked() ||
            m_channel->hasPurge() != m_ps->m_purgeCheck->isChecked()     ||
            m_channel->purgeCount() != m_ps->m_purgeCountSpinBox->value() ||
            fetchTypeChanged ||
            m_channel->writeTags() != m_ps->m_writeTagsCheck->isChecked()
          );
}

void
PodcastSettingsDialog::checkModified() //slot
{
    buttonBox()->button(QDialogButtonBox::Apply)->setEnabled( hasChanged() );
}

void
PodcastSettingsDialog::slotApply()       //slot
{
    m_channel->setUrl( QUrl( m_ps->m_urlLineEdit->text() ) );
    m_channel->setAutoScan( m_ps->m_autoFetchCheck->isChecked() );
    m_channel->setFetchType(
        m_ps->m_downloadRadio->isChecked() ?
        Podcasts::PodcastChannel::DownloadWhenAvailable :
        Podcasts::PodcastChannel::StreamOrDownloadOnDemand
    );
    m_channel->setSaveLocation( m_ps->m_saveLocation->url() );

    m_channel->setPurge( m_ps->m_purgeCheck->isChecked() );
    m_channel->setPurgeCount( m_ps->m_purgeCountSpinBox->value() );
    m_channel->setWriteTags( m_ps->m_writeTagsCheck->isChecked() );

    buttonBox()->button(QDialogButtonBox::Apply)->setEnabled( false );
}

bool PodcastSettingsDialog::configure()
{
    return exec() == QDialog::Accepted;
}

void PodcastSettingsDialog::launchFilenameLayoutConfigDialog()
{
    PodcastFilenameLayoutConfigDialog pflcDialog( m_channel, this );
    pflcDialog.configure();
}



