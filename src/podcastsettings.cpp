// (c) 2005-2006 Seb Ruiz <me@sebruiz.net>
// (c) 2006 Bart Cerneels <shanachie@yucom.be>
// See COPYING file for licensing information.

#include "mediabrowser.h"
#include "podcastsettingsbase.h"
#include "podcastsettings.h"

#include <klineedit.h>
#include <knuminput.h>
#include <kmessagebox.h>    //global changes confirmation
#include <kurlrequester.h>
#include <klocale.h>
#include <kurl.h>
#include <kwin.h>

#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qlabel.h>

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
                            : KDialogBase(  parent, 0, true, i18n("change options", "Configure %1").arg( settings->m_title )
                            , KDialogBase::User1|KDialogBase::Ok|KDialogBase::Cancel
                            , KDialogBase::Ok, true
                            , KGuiItem(i18n("Reset"), "reset" ) )
        , m_settings( settings )
{
    init();
    setSettings( settings );
}

PodcastSettingsDialog::PodcastSettingsDialog( const QPtrList<PodcastSettings> &list, const QString &caption, QWidget* parent )
    : KDialogBase(  parent, 0, true, i18n("change options", "Configure %1").arg( caption )
            , KDialogBase::User1|KDialogBase::Ok|KDialogBase::Cancel
                    , KDialogBase::Ok, true
                    , KGuiItem(i18n("Reset"), "reset" ) )
        , m_settingsList( list )
{
    init();
    m_settings = m_settingsList.first();
    if( !m_settings->m_saveLocation.endsWith( "/" ) )
        m_settings->m_saveLocation = m_settings->m_saveLocation.section( "/", 0, -2 );
    setSettings( m_settings );
}

void
PodcastSettingsDialog::init()
{
        m_ps = new PodcastSettingsDialogBase(this);

        KWin::setState( winId(), NET::SkipTaskbar );

        setMainWidget(m_ps);
        m_ps->m_saveLocation->setMode( KFile::Directory | KFile::ExistingOnly );

        m_ps->m_addToMediaDeviceCheck->setEnabled( MediaBrowser::isAvailable() );

        enableButtonOK( false );

         // Connects for modification check
        connect( m_ps->m_purgeCountSpinBox->child( "qt_spinbox_edit" ),  SIGNAL(textChanged( const QString& )), SLOT(checkModified()) );
        connect( m_ps->m_saveLocation,   SIGNAL(textChanged( const QString& )), SLOT(checkModified()) );
        connect( m_ps->m_autoFetchCheck, SIGNAL(clicked()),                     SLOT(checkModified()) );
        connect( m_ps->m_streamRadio,    SIGNAL(clicked()),                     SLOT(checkModified()) );
        connect( m_ps->m_addToMediaDeviceCheck, SIGNAL(clicked()),              SLOT(checkModified()) );
        connect( m_ps->m_downloadRadio,  SIGNAL(clicked()),                     SLOT(checkModified()) );
        connect( m_ps->m_purgeCheck,     SIGNAL(clicked()),                     SLOT(checkModified()) );
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
    enableButtonOK( hasChanged() );
}

void PodcastSettingsDialog::slotOk()       //slot
{
    enableButtonOK( false ); //visual feedback

    if ( !m_settingsList.isEmpty() )
    {
        foreachType( QPtrList<PodcastSettings>, m_settingsList)
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

    KDialogBase::slotOk();
}

// KUrlRequester doesn't provide us with convenient functions for adding trailing slashes
QString PodcastSettingsDialog::requesterSaveLocation()
{
    QString url = m_ps->m_saveLocation->url();
    if( url.endsWith( "/" ) )
        return url;
    else
        return url + '/';
}

void PodcastSettingsDialog::setSettings( PodcastSettings *settings )
{
    QString saveLocation = settings->m_saveLocation;

    m_ps->m_saveLocation->setURL( saveLocation );
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
    setSettings( new PodcastSettings(m_settings->m_title) );
    checkModified();
}

bool PodcastSettingsDialog::configure()
{
    return exec() == QDialog::Accepted;
}

#include "podcastsettings.moc"
