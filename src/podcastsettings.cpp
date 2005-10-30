// (c) 2005 Seb Ruiz <me@sebruiz.net>
// See COPYING file for licensing information.

#include "podcastsettings.h"
#include "podcastsettingsbase.h"

#include <klineedit.h>
#include <knuminput.h>
#include <kmessagebox.h>    //global changes confirmation
#include <kurlrequester.h>
#include <klocale.h>
#include <kwin.h>

#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qradiobutton.h>

PodcastSettings::PodcastSettings( const QString& url, const QString& save, bool autoScan, int interval,
                                  int fetch, bool purge, int purgeCount, QWidget* parent )
                    : KDialogBase(  parent, 0, true, i18n("Configure Podcast Stream"),
                                    KDialogBase::User1|KDialogBase::Ok|KDialogBase::Cancel,
                                    KDialogBase::Ok, true,
                                    KGuiItem(i18n("Apply to all Podcasts"), "apply" ) )
                    , m_ps( new PodcastSettingsDialogBase(this) )
                    , m_url( url )
                    , m_save( save )
                    , m_autoScan( autoScan )
                    , m_interval( interval )
                    , m_fetch( fetch )
                    , m_purge( purge )
                    , m_purgeCount( purgeCount )
                    , m_applyToAll( false )
{
    KWin::setState( winId(), NET::SkipTaskbar );

    setMainWidget(m_ps);
    m_ps->m_urlLine->setText( url );
    m_ps->m_saveLocation->setMode( KFile::Directory | KFile::ExistingOnly );
    m_ps->m_saveLocation->setURL( save );

    m_ps->m_autoFetchCheck->setChecked( autoScan );

    if( fetch == STREAM )
    {
        m_ps->m_streamRadio->setChecked( true );
        m_ps->m_downloadRadio->setChecked( false );
    }
    else if( fetch == AUTOMATIC )
    {
        m_ps->m_streamRadio->setChecked( false );
        m_ps->m_downloadRadio->setChecked( true );
    }

    m_ps->m_purgeCheck->setChecked( purge );
    m_ps->m_purgeCountSpinBox->setValue( purgeCount );

    if( !purge )
        m_ps->m_purgeCountSpinBox->setEnabled( false );

    enableButtonOK( false );

    connect( m_ps->m_purgeCountSpinBox->child( "qt_spinbox_edit" ),  SIGNAL(textChanged( const QString& )), SLOT(checkModified()) );

    // Connects for modification check
    connect( m_ps->m_urlLine,        SIGNAL(textChanged( const QString& )), SLOT(checkModified()) );
    connect( m_ps->m_saveLocation,   SIGNAL(textChanged( const QString& )), SLOT(checkModified()) );
    connect( m_ps->m_autoFetchCheck, SIGNAL(clicked()),                     SLOT(checkModified()) );
    connect( m_ps->m_streamRadio,    SIGNAL(clicked()),                     SLOT(checkModified()) );
    connect( m_ps->m_downloadRadio,  SIGNAL(clicked()),                     SLOT(checkModified()) );
    connect( m_ps->m_purgeCheck,     SIGNAL(clicked()),                     SLOT(checkModified()) );
}

bool
PodcastSettings::hasChanged()
{
    bool fetchTypeChanged = true;

    if( m_ps->m_streamRadio->isChecked()          && m_fetch == STREAM   ||
        m_ps->m_downloadRadio->isChecked()        && m_fetch == AUTOMATIC  )

        fetchTypeChanged = false;

    return  !m_ps->m_urlLine->text().isEmpty() &&
           ( m_url             != m_ps->m_urlLine->text()             ||
             m_save            != requesterSaveLocation()             ||
             m_autoScan        != m_ps->m_autoFetchCheck->isChecked() ||
             m_purge           != m_ps->m_purgeCheck->isChecked()     ||
             m_purgeCount      != m_ps->m_purgeCountSpinBox->value()  ||
             fetchTypeChanged );

}

void
PodcastSettings::checkModified() //slot
{
    enableButtonOK( hasChanged() );
}

void
PodcastSettings::slotOk()       //slot
{
    enableButtonOK( false ); //visual feedback

    m_url             = m_ps->m_urlLine->text();
    m_save            = requesterSaveLocation();
    m_autoScan        = m_ps->m_autoFetchCheck->isChecked();
    m_purge           = m_ps->m_purgeCheck->isChecked();
    m_purgeCount      = m_ps->m_purgeCountSpinBox->value();

    if( m_ps->m_streamRadio->isChecked() )
        m_fetch = STREAM;
    else
        m_fetch = AUTOMATIC;

    KDialogBase::slotOk();
}

void
PodcastSettings::slotUser1()    //slot
{
    int button = KMessageBox::warningContinueCancel( this, i18n( "<p>This will set podcast settings globally. Are you sure?" ) );

    if ( button == KMessageBox::Continue )
    {
        // TODO We need to check which files have been deleted successfully
        m_applyToAll = true;
        slotOk();
        return;
    }
}

// KUrlRequester doesn't provide us with convenient functions for adding trailing slashes
QString
PodcastSettings::requesterSaveLocation()
{
    QString url = m_ps->m_saveLocation->url();
    if( url.endsWith( "/" ) )
        return url;
    else
        return url + "/";
}

#include "podcastsettings.moc"
