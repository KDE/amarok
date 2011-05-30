#include "PodcastFilenameLayoutConfigDialog.h"
#include "ui_PodcastFilenameLayoutConfigWidget.h"

#include <QApplication>

PodcastFilenameLayoutConfigDialog::PodcastFilenameLayoutConfigDialog( Podcasts::SqlPodcastChannelPtr channel, QWidget* parent )
    : KDialog( parent )
    , m_channel( channel )
    , m_pflc( new Ui::PodcastFilenameLayoutConfigWidget )
{
    QWidget* main = new QWidget( this );
    m_pflc->setupUi( main );
    setMainWidget( main );

    setCaption( i18nc( "Change filename layout", "Podcast Episode Filename Configuration" ) );
    setModal( true );
    setButtons( Cancel | Ok );
    setDefaultButton( Ok );
    showButtonSeparator( true );
    setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Fixed );

    init();
}

void PodcastFilenameLayoutConfigDialog::init()
{
    //initialize state of the various gui items based on the channel settings

    QString filenameLayout = m_channel->filenameLayout();

    if( filenameLayout == QLatin1String( "%default%" ) )
    {
        m_pflc->m_filenameLayoutDefault->setChecked( true );
        m_pflc->m_filenameLayoutCustom->setChecked( false );
        choice = 0;
    }
    else
    {
        m_pflc->m_filenameLayoutDefault->setChecked( false );
        m_pflc->m_filenameLayoutCustom->setChecked( true );
        m_pflc->m_filenameLayoutText->setText( filenameLayout );
        choice = 1;
    }

    connect( this, SIGNAL( okClicked() ), this, SLOT( slotApply() ) );
}


void PodcastFilenameLayoutConfigDialog::slotApply()
{
    if( m_pflc->m_filenameLayoutCustom->isChecked() )
    {
        m_channel->setFilenameLayout( m_pflc->m_filenameLayoutText->text() );
    }
    else
    {
        m_channel->setFilenameLayout( "%default%" );
    }
}

bool PodcastFilenameLayoutConfigDialog::configure()
{
    return exec() == QDialog::Accepted;
}


#include "PodcastFilenameLayoutConfigDialog.moc"