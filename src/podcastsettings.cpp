/******************************************************************************
 * Copyright (C) 2005-2006 Seb Ruiz <ruiz@kde.org>                            *
 *           (C) 2006 Bart Cerneels <shanachie@yucom.be>                      *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#include "podcastsettings.h"

#include "Amarok.h"

#include <KLineEdit>
#include <KLocale>
#include <KMessageBox>    //global changes confirmation
#include <KNumInput>
#include <KUrl>
#include <KUrlRequester>
#include <KWindowSystem>

#include <QCheckBox>
#include <QDomNode>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QTextEdit>

PodcastSettings::PodcastSettings( const QDomNode &channelSettings, const QString &title )
    : m_title( title )
{
    m_saveLocation = channelSettings.namedItem( "savelocation").toElement().text();
    m_autoScan = channelSettings.namedItem( "autoscan").toElement().text() == "true";
    m_fetch = channelSettings.namedItem("fetch").toElement().text() == "automatic"?AUTOMATIC:STREAM;
    m_addToMediaDevice = channelSettings.namedItem( "autotransfer").toElement().text() == "true";
    m_purge = channelSettings.namedItem( "purge").toElement().text() == "true";
    m_purgeCount = channelSettings.namedItem( "purgecount").toElement().text().toInt();
}

// default settings
PodcastSettings::PodcastSettings( const QString &title )
    : m_title( title )
{
    m_saveLocation = Amarok::saveLocation( "podcasts/" );
    m_saveLocation += Amarok::vfatPath( m_title );
    m_autoScan = true;
    m_fetch = STREAM;
    m_addToMediaDevice = false;
    m_purge = false;
    m_purgeCount = 0;
}

PodcastSettings::PodcastSettings( const QString &title, const QString &save, const bool autoScan,
                                  const int fetchType, const bool autotransfer, const bool purge, const int purgecount )
{
    m_title = title;
    if( save.isEmpty() )
    {
        m_saveLocation = Amarok::saveLocation( "podcasts/" );
        m_saveLocation += Amarok::vfatPath( m_title );
    }
    else
        m_saveLocation = save;

    m_autoScan = autoScan;
    m_fetch = fetchType;
    m_addToMediaDevice = autotransfer;
    m_purge = purge;
    m_purgeCount = purgecount;
}

PodcastSettingsDialog::PodcastSettingsDialog( PodcastSettings *settings, QWidget* parent )
                            : KDialog( parent )
        , m_settings( settings )
{
    setCaption( i18nc("change options", "Configure %1", settings->m_title ) );
    setModal( true );
    setButtons( Ok | Cancel | User1 );
    setButtonGuiItem( User1, KGuiItem( i18n("Reset") ) );
    setDefaultButton( Ok );
    showButtonSeparator( true );
    init();
    setSettings( settings );
}

PodcastSettingsDialog::PodcastSettingsDialog( const QList<PodcastSettings *> list, const QString &caption, QWidget* parent )
    : KDialog(  parent )
        , m_settingsList( list )
{
    setCaption( i18nc("change options", "Configure %1", caption ) );
    setModal( true );
    setButtons( Ok | Cancel | User1 );
    setButtonGuiItem( User1, KGuiItem( i18n("Reset") ) );
    setDefaultButton( Ok );
    showButtonSeparator( true );

    init();
    m_settings = m_settingsList.first();
    if( !m_settings->m_saveLocation.endsWith( '/' ) )
        m_settings->m_saveLocation = m_settings->m_saveLocation.section( "/", 0, -2 );
    setSettings( m_settings );
}

void
PodcastSettingsDialog::init()
{
        m_ps = new PodcastSettingsDialogBase(this);

#ifdef Q_WS_X11
        KWindowSystem::setState( winId(), NET::SkipTaskbar );
#endif

        setMainWidget(m_ps);
        m_ps->m_saveLocation->setMode( KFile::Directory | KFile::ExistingOnly );

        //m_ps->m_addToMediaDeviceCheck->setEnabled( MediaBrowser::isAvailable() );

        enableButtonOk( false );

         // Connects for modification check
        connect( m_ps->m_purgeCountSpinBox->findChild<QTextEdit *>( "qt_spinbox_edit" ),  SIGNAL(textChanged( const QString& )), SLOT(checkModified()) );
        connect( m_ps->m_saveLocation,   SIGNAL(textChanged( const QString& )), SLOT(checkModified()) );
        connect( m_ps->m_autoFetchCheck, SIGNAL(clicked()),                     SLOT(checkModified()) );
        connect( m_ps->m_streamRadio,    SIGNAL(clicked()),                     SLOT(checkModified()) );
        connect( m_ps->m_addToMediaDeviceCheck, SIGNAL(clicked()),              SLOT(checkModified()) );
        connect( m_ps->m_downloadRadio,  SIGNAL(clicked()),                     SLOT(checkModified()) );
        connect( m_ps->m_purgeCheck,     SIGNAL(clicked()),                     SLOT(checkModified()) );
	connect(this,SIGNAL(okClicked()),this,SLOT(slotOk()));
	connect(this,SIGNAL(user1Clicked()), this,SLOT(slotUser1()));
}

bool
PodcastSettingsDialog::hasChanged()
{
    bool fetchTypeChanged = true;

    if( m_ps->m_streamRadio->isChecked()   && m_settings->m_fetch == STREAM   ||
        m_ps->m_downloadRadio->isChecked() && m_settings->m_fetch == AUTOMATIC  )

        fetchTypeChanged = false;

    return( m_settings->m_saveLocation     != requesterSaveLocation()             ||
            m_settings->m_autoScan         != m_ps->m_autoFetchCheck->isChecked() ||
            m_settings->m_addToMediaDevice != m_ps->m_addToMediaDeviceCheck->isChecked() ||
            m_settings->m_purge            != m_ps->m_purgeCheck->isChecked()     ||
            m_settings->m_purgeCount       != m_ps->m_purgeCountSpinBox->value()  ||
            fetchTypeChanged );
}

void
PodcastSettingsDialog::checkModified() //slot
{
    enableButtonOk( hasChanged() );
}

void PodcastSettingsDialog::slotOk()       //slot
{
    enableButtonOk( false ); //visual feedback

    if ( !m_settingsList.isEmpty() )
    {
        oldForeachType( QList<PodcastSettings *>, m_settingsList)
        {
            (*it)->m_saveLocation     = requesterSaveLocation().append( Amarok::vfatPath( (*it)->title() ) );
            (*it)->m_autoScan         = m_ps->m_autoFetchCheck->isChecked();
            (*it)->m_addToMediaDevice = m_ps->m_addToMediaDeviceCheck->isChecked();
            (*it)->m_purge            = m_ps->m_purgeCheck->isChecked();
            (*it)->m_purgeCount       = m_ps->m_purgeCountSpinBox->value();
            if( m_ps->m_streamRadio->isChecked() )
                (*it)->m_fetch = STREAM;
            else
                (*it)->m_fetch = AUTOMATIC;
        }
    }
    else
    {
        m_settings->m_saveLocation     = requesterSaveLocation();
        m_settings->m_autoScan         = m_ps->m_autoFetchCheck->isChecked();
        m_settings->m_addToMediaDevice = m_ps->m_addToMediaDeviceCheck->isChecked();
        m_settings->m_purge            = m_ps->m_purgeCheck->isChecked();
        m_settings->m_purgeCount       = m_ps->m_purgeCountSpinBox->value();

        if( m_ps->m_streamRadio->isChecked() )
            m_settings->m_fetch = STREAM;
        else
            m_settings->m_fetch = AUTOMATIC;
    }
}

// KUrlRequester doesn't provide us with convenient functions for adding trailing slashes
QString PodcastSettingsDialog::requesterSaveLocation()
{
    return m_ps->m_saveLocation->url().path( KUrl::AddTrailingSlash );
}

void PodcastSettingsDialog::setSettings( PodcastSettings *settings )
{
    QString saveLocation = settings->m_saveLocation;

    m_ps->m_saveLocation->setUrl( saveLocation );
    m_ps->m_autoFetchCheck->setChecked( settings->m_autoScan );

    if( settings->m_fetch == STREAM )
    {
        m_ps->m_streamRadio->setChecked( true );
        m_ps->m_downloadRadio->setChecked( false );
    }
    else if( settings->m_fetch == AUTOMATIC )
    {
        m_ps->m_streamRadio->setChecked( false );
        m_ps->m_downloadRadio->setChecked( true );
    }

    m_ps->m_addToMediaDeviceCheck->setChecked( settings->m_addToMediaDevice );
    m_ps->m_purgeCheck->setChecked( settings->m_purge );
    m_ps->m_purgeCountSpinBox->setValue( settings->m_purgeCount );

    if( !settings->m_purge )
    {
        m_ps->m_purgeCountSpinBox->setEnabled( false );
        m_ps->m_purgeCountLabel->setEnabled( false );
    }
}

//reset to default settings button
void PodcastSettingsDialog::slotUser1()    //slot
{
    PodcastSettings settings( m_settings->m_title );
    setSettings( &settings );
    checkModified();
}

bool PodcastSettingsDialog::configure()
{
    return exec() == QDialog::Accepted;
}

#include "podcastsettings.moc"
