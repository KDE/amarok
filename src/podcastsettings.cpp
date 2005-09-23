// (c) 2005 Seb Ruiz <me@sebruiz.net>
// See COPYING file for licensing information.

#include "podcastsettings.h"

#include <klineedit.h>
#include <knuminput.h>
#include <kurlrequester.h>
#include <kwin.h>

#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qradiobutton.h>

PodcastSettings::PodcastSettings( QString& url, QString& save, bool &autoScan, int &interval,
                                  int &fetch, bool &purge, int &purgeCount, QWidget* parent )
                    : PodcastSettingsDialogBase( parent )
                    , m_url( url )
                    , m_save( save )
                    , m_autoScan( autoScan )
                    , m_interval( interval )
                    , m_fetch( fetch )
                    , m_purge( purge )
                    , m_purgeCount( purgeCount )
{
    // Gives the window a small title bar, and skips a taskbar entry
    KWin::setType( winId(), NET::Utility );
    KWin::setState( winId(), NET::SkipTaskbar );

    m_urlLine->setText( url );
    m_saveLocation->setMode( KFile::Directory | KFile::ExistingOnly );
    m_saveLocation->setURL( save );

    m_autoFetchCheck->setChecked( autoScan );

    if( fetch == DOWNLOAD )
    {
        m_downloadRequestRadio->setChecked( true );
        m_streamRadio->setChecked( false );
        m_downloadRadio->setChecked( false );
    }
    else if( fetch == STREAM )
    {
        m_downloadRequestRadio->setChecked( false );
        m_streamRadio->setChecked( true );
        m_downloadRadio->setChecked( false );
    }
    else if( fetch == AVAILABLE )
    {
        m_downloadRequestRadio->setChecked( false );
        m_streamRadio->setChecked( false );
        m_downloadRadio->setChecked( true );
    }

    m_purgeCheck->setChecked( purge );
    m_purgeCountSpinBox->setValue( purgeCount );

    if( !purge )
        m_purgeCountSpinBox->setEnabled( false );
    pushButton_ok->setEnabled( false );

    connect( m_purgeCountSpinBox->child( "qt_spinbox_edit" ),  SIGNAL(textChanged( const QString& )), SLOT(checkModified()) );

    // Connects for modification check
    connect( m_urlLine,        SIGNAL(textChanged( const QString& )), SLOT(checkModified()) );
    connect( m_saveLocation,   SIGNAL(textChanged( const QString& )), SLOT(checkModified()) );
    connect( m_autoFetchCheck, SIGNAL(clicked()),                     SLOT(checkModified()) );
    connect( m_streamRadio,    SIGNAL(clicked()),                     SLOT(checkModified()) );
    connect( m_downloadRadio,  SIGNAL(clicked()),                     SLOT(checkModified()) );
    connect( m_downloadRequestRadio,  SIGNAL(clicked()),              SLOT(checkModified()) );
    connect( m_purgeCheck,     SIGNAL(clicked()),                     SLOT(checkModified()) );


    connect( pushButton_cancel,SIGNAL(clicked()), SLOT(cancelPressed()) );
    connect( pushButton_ok,    SIGNAL(clicked()), SLOT(accept()) );
}

bool
PodcastSettings::hasChanged()
{
    bool fetchTypeChanged = true;

    if( m_downloadRequestRadio->isChecked() && m_fetch == DOWNLOAD  ||
        m_streamRadio->isChecked()          && m_fetch == STREAM    ||
        m_downloadRadio->isChecked()        && m_fetch == AVAILABLE  )

        fetchTypeChanged = false;

    return  !m_urlLine->text().isEmpty() &&
           ( m_url             != m_urlLine->text()             ||
             m_save            != m_saveLocation->url()         ||
             m_autoScan        != m_autoFetchCheck->isChecked() ||
             m_purge           != m_purgeCheck->isChecked()     ||
             m_purgeCount      != m_purgeCountSpinBox->value()  ||
             fetchTypeChanged );

}

void
PodcastSettings::checkModified() //slot
{
    pushButton_ok->setEnabled( hasChanged() );
}

void
PodcastSettings::cancelPressed() //slot
{
    reject();
}

void
PodcastSettings::accept()       //slot
{
    pushButton_ok->setEnabled( false ); //visual feedback

    m_url             = m_urlLine->text();
    m_save            = m_saveLocation->url();
    m_autoScan        = m_autoFetchCheck->isChecked();
    m_purge           = m_purgeCheck->isChecked();
    m_purgeCount      = m_purgeCountSpinBox->value();

    if( m_streamRadio->isChecked() )
        m_fetch = STREAM;
    else if( m_downloadRequestRadio->isChecked() )
        m_fetch = DOWNLOAD;
    else
        m_fetch = AVAILABLE;

    QDialog::accept();
}

#include "podcastsettings.moc"
