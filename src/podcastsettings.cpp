// (c) 2005 Seb Ruiz <me@sebruiz.net>
// See COPYING file for licensing information.

#include "podcastsettings.h"
#include "podcastsettingsbase.h"
#include "mediabrowser.h"

#include <klineedit.h>
#include <knuminput.h>
#include <kmessagebox.h>    //global changes confirmation
#include <kurlrequester.h>
#include <klocale.h>
#include <kwin.h>

#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qlabel.h>

PodcastSettings::PodcastSettings( const QString& url, const QString& save, bool autoScan, int interval,
                                  int fetch, bool addToMediaDevice, bool purge, int purgeCount )
                    : m_url( url )
                    , m_save( save )
                    , m_autoScan( autoScan )
                    , m_interval( interval )
                    , m_fetch( fetch )
                    , m_addToMediaDevice( addToMediaDevice )
                    , m_purge( purge )
                    , m_purgeCount( purgeCount )
{
}

PodcastSettings::PodcastSettings( const QDomNode &channelSettings )
{
    m_url = channelSettings.namedItem( "url").toElement().text();
    m_save = channelSettings.namedItem( "savelocation").toElement().text();
    m_autoScan = channelSettings.namedItem( "autoscan").toElement().text() == "true";
    m_interval = channelSettings.namedItem( "scaninterval").toElement().text().toInt();
    m_fetch = channelSettings.namedItem("fetch").toElement().text() == "automatic"?AUTOMATIC:STREAM;
    m_addToMediaDevice = channelSettings.namedItem( "autotransfer").toElement().text() == "true";
    m_purge = channelSettings.namedItem( "purge").toElement().text() == "true";
    m_purgeCount = channelSettings.namedItem( "purgecount").toElement().text().toInt();

}

PodcastSettings::PodcastSettings()
{
    m_url = "file:///";
    m_save = amaroK::saveLocation( "podcasts/data/" );
    m_autoScan = false;
    m_interval = 4;
    m_fetch = STREAM;
    m_addToMediaDevice = false;
    m_purge = false;
    m_purgeCount = 2;
}

const QDomElement
PodcastSettings::xml()
{
    QDomDocument doc;
    QDomElement i = doc.createElement("settings");

    QDomElement attr = doc.createElement( "savelocation" );
    QDomText t = doc.createTextNode( m_save );
    attr.appendChild( t );
    i.appendChild( attr );

    attr = doc.createElement( "autoscan" );
    t = doc.createTextNode( m_autoScan ? "true" : "false" );
    attr.appendChild( t );
    i.appendChild( attr );

    attr = doc.createElement( "scaninterval" );
    t = doc.createTextNode( QString::number( m_interval ) );
    attr.appendChild( t );
    i.appendChild( attr );

    attr = doc.createElement( "fetch" );
    t = doc.createTextNode( ( m_fetch == AUTOMATIC ) ? "automatic" : "stream" );
    attr.appendChild( t );
    i.appendChild( attr );

    attr = doc.createElement( "autotransfer" );
    t = doc.createTextNode( ( m_addToMediaDevice ) ? "true" : "false" );
    attr.appendChild( t );
    i.appendChild( attr );

    attr = doc.createElement( "purge" );
    t = doc.createTextNode( m_purge ? "true" : "false" );
    attr.appendChild( t );
    i.appendChild( attr );

    attr = doc.createElement( "purgecount" );
    t = doc.createTextNode( QString::number( m_purgeCount ) );
    attr.appendChild( t );
    i.appendChild( attr );

    return i;
}


PodcastSettingsDialog::PodcastSettingsDialog( PodcastSettings *settings, PodcastSettings *parentSettings, QWidget* parent )
            :KDialogBase(  parent, 0, true, i18n("Configure Podcast Stream"),
                            KDialogBase::User1|KDialogBase::Ok|KDialogBase::Cancel,
                            KDialogBase::Ok, true,
                            KGuiItem(i18n("reset"), "reset" ) )
        , m_settings( settings )
        , m_parentSettings( parentSettings )
{
        m_ps = new PodcastSettingsDialogBase(this);

        KWin::setState( winId(), NET::SkipTaskbar );

        setMainWidget(m_ps);
        m_ps->m_urlLine->setText( settings->m_url );
        m_ps->m_saveLocation->setMode( KFile::Directory | KFile::ExistingOnly );
        m_ps->m_saveLocation->setURL( settings->m_save );

        m_ps->m_autoFetchCheck->setChecked( settings->m_autoScan );

        if( m_settings->m_fetch == STREAM )
        {
            m_ps->m_streamRadio->setChecked( true );
            m_ps->m_downloadRadio->setChecked( false );
        }
        else if( m_settings->m_fetch == AUTOMATIC )
        {
            m_ps->m_streamRadio->setChecked( false );
            m_ps->m_downloadRadio->setChecked( true );
        }

        m_ps->m_addToMediaDeviceCheck->setChecked( m_settings->m_addToMediaDevice );
        m_ps->m_addToMediaDeviceCheck->setEnabled( MediaBrowser::isAvailable() );

        m_ps->m_purgeCheck->setChecked( m_settings->m_purge );
        m_ps->m_purgeCountSpinBox->setValue( m_settings->m_purgeCount );

        if( !m_settings->m_purge )
        {
            m_ps->m_purgeCountSpinBox->setEnabled( false );
            m_ps->m_purgeCountLabel->setEnabled( false );
        }

        enableButtonOK( false );

        connect( m_ps->m_purgeCountSpinBox->child( "qt_spinbox_edit" ),  SIGNAL(textChanged( const QString& )), SLOT(checkModified()) );

    // Connects for modification check
        connect( m_ps->m_urlLine,        SIGNAL(textChanged( const QString& )), SLOT(checkModified()) );
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

    if( m_ps->m_streamRadio->isChecked()          && m_settings->m_fetch == STREAM   ||
        m_ps->m_downloadRadio->isChecked()        && m_settings->m_fetch == AUTOMATIC  )

        fetchTypeChanged = false;

    return  !m_ps->m_urlLine->text().isEmpty() &&
            ( m_settings->m_url             != m_ps->m_urlLine->text()             ||
            m_settings->m_save            != requesterSaveLocation()             ||
            m_settings->m_autoScan        != m_ps->m_autoFetchCheck->isChecked() ||
            m_settings->m_addToMediaDevice!= m_ps->m_addToMediaDeviceCheck->isChecked() ||
            m_settings->m_purge           != m_ps->m_purgeCheck->isChecked()     ||
            m_settings->m_purgeCount      != m_ps->m_purgeCountSpinBox->value()  ||
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

    m_settings->m_url             = m_ps->m_urlLine->text();
    m_settings->m_save            = requesterSaveLocation();
    m_settings->m_autoScan        = m_ps->m_autoFetchCheck->isChecked();
    m_settings->m_addToMediaDevice= m_ps->m_addToMediaDeviceCheck->isChecked();
    m_settings->m_purge           = m_ps->m_purgeCheck->isChecked();
    m_settings->m_purgeCount      = m_ps->m_purgeCountSpinBox->value();

    if( m_ps->m_streamRadio->isChecked() )
        m_settings->m_fetch = STREAM;
    else
        m_settings->m_fetch = AUTOMATIC;

    KDialogBase::slotOk();
}

// KUrlRequester doesn't provide us with convenient functions for adding trailing slashes
QString PodcastSettingsDialog::requesterSaveLocation()
{
    QString url = m_ps->m_saveLocation->url();
    if( url.endsWith( "/" ) )
        return url;
    else
        return url + "/";
}

//Apply to all button
void PodcastSettingsDialog::slotUser1()    //slot
{
}

#include "podcastsettings.moc"
