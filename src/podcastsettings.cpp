// (c) 2005 Seb Ruiz <me@sebruiz.net>
// See COPYING file for licensing information.

#include "podcastsettings.h"

#include <klineedit.h>
#include <knuminput.h>

#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qradiobutton.h>

PodcastSettings::PodcastSettings( KURL& url, bool &autoScan, int &interval,
                                  int &fetch, bool &purge, int &purgeCount, QWidget* parent )
                    : PodcastSettingsDialogBase( parent )
                    , m_url( url )
                    , m_autoScan( autoScan )
                    , m_interval( interval )
                    , m_fetch( fetch )
                    , m_purge( purge )
                    , m_purgeCount( purgeCount )
{
    m_urlLine->setText( url.prettyURL() );
    m_autoFetchCheck->setChecked( autoScan );
    m_intervalSpinBox->setValue( interval );

    if( fetch == STREAM )
    {
        m_streamRadio->setChecked( true );
        m_downloadRadio->setChecked( false );
    }
    else
    {
        m_streamRadio->setChecked( false );
        m_downloadRadio->setChecked( true );
    }

    m_purgeCheck->setChecked( purge );
    m_purgeCountSpinBox->setValue( purgeCount );

    if( !purge )
        m_purgeCountSpinBox->setEnabled( false );
    if( !autoScan )
        m_intervalSpinBox->setEnabled( false );

    pushButton_ok->setEnabled( false );

    connect( m_purgeCountSpinBox->child( "qt_spinbox_edit" ),  SIGNAL(textChanged( const QString& )), SLOT(checkModified()) );
    connect( m_intervalSpinBox->child( "qt_spinbox_edit" ),    SIGNAL(textChanged( const QString& )), SLOT(checkModified()) );


    // Connects for modification check
    connect( m_urlLine,        SIGNAL(textChanged( const QString& )), SLOT(checkModified()) );
    connect( m_autoFetchCheck, SIGNAL(clicked()),                     SLOT(checkModified()) );
    connect( m_streamRadio,    SIGNAL(clicked()),                     SLOT(checkModified()) );
    connect( m_downloadRadio,  SIGNAL(clicked()),                     SLOT(checkModified()) );
    connect( m_purgeCheck,     SIGNAL(clicked()),                     SLOT(checkModified()) );


    connect( pushButton_cancel,SIGNAL(clicked()), SLOT(cancelPressed()) );
    connect( pushButton_ok,    SIGNAL(clicked()), SLOT(accept()) );
}

bool
PodcastSettings::hasChanged()
{
    bool fetchTypeChanged = false;

    if( m_streamRadio->isChecked() && m_fetch == DOWNLOAD )
        fetchTypeChanged = true;

    return ( m_url.prettyURL() != m_urlLine->text() ||
             m_autoScan        != m_autoFetchCheck->isChecked() ||
             m_interval        != m_intervalSpinBox->value() ||
             m_purge           != m_purgeCheck->isChecked() ||
             m_purgeCount      != m_purgeCountSpinBox->value() ||
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

    m_url             = KURL( m_urlLine->text() );
    m_autoScan        = m_autoFetchCheck->isChecked();
    m_interval        = m_intervalSpinBox->value();
    m_purge           = m_purgeCheck->isChecked();
    m_purgeCount      = m_purgeCountSpinBox->value();

    QDialog::accept();
}

#include "podcastsettings.moc"
