#include "nmm_configdialog.h"
#include "nmm_kdeconfig.h"
#include "debug.h"

#include <qbuttongroup.h>
#include <qinputdialog.h>
#include <qlistbox.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <kcombobox.h>

NmmConfigDialog::NmmConfigDialog()
    : PluginConfig()
{
    kdDebug() << k_funcinfo << endl;

    m_view = new NmmConfigDialogBase();

    if ( NmmKDEConfig::audioOutputPlugin() == "ALSAPlaybackNode" )
        m_view->audioPlaybackNode->setCurrentItem( 1 );

    m_view->audioGroup->setButton( NmmKDEConfig::audioLocation() );
    setCheckedAudioList( m_view->videoHostListButton->isChecked() );
    m_view->audioListBox->insertStringList( NmmKDEConfig::audioHost() );
    
    m_view->videoGroup->setButton( NmmKDEConfig::videoLocation() );
    setCheckedVideoList( m_view->videoHostListButton->isChecked() );
    m_view->videoListBox->insertStringList( NmmKDEConfig::videoHost() );
    
    connect( m_view->addAudioLocationButton, SIGNAL( released() ), SLOT( addAudioHost() ) );
    connect( m_view->removeAudioLocationButton, SIGNAL( released() ), SLOT( removeAudioHost() ) );
    connect( m_view->audioHostListButton, SIGNAL( toggled(bool) ), SLOT( setCheckedAudioList(bool) ) );

    connect( m_view->addVideoLocationButton, SIGNAL( released() ), SLOT( addVideoHost() ) );
    connect( m_view->removeVideoLocationButton, SIGNAL( released() ), SLOT( removeVideoHost() ) );
    connect( m_view->videoHostListButton, SIGNAL( toggled(bool) ), SLOT( setCheckedVideoList(bool) ) );

    PluginConfig::connect( m_view->audioPlaybackNode, SIGNAL( activated( int ) ), SIGNAL( viewChanged() ) );
    PluginConfig::connect( m_view->audioGroup, SIGNAL( released( int ) ), SIGNAL( viewChanged() ) );
    PluginConfig::connect( m_view->videoGroup, SIGNAL( released( int ) ), SIGNAL( viewChanged() ) );

}

NmmConfigDialog::~NmmConfigDialog()
{
    kdDebug() << k_funcinfo << endl;
    delete m_view;
}


bool NmmConfigDialog::hasChanged() const
{
    return NmmKDEConfig::audioOutputPlugin() != m_view->audioPlaybackNode->currentText() ||
           NmmKDEConfig::audioLocation() != m_view->audioGroup->selectedId() ||
           NmmKDEConfig::audioHost() != hostlist(m_view->audioListBox) ||
           NmmKDEConfig::videoLocation() != m_view->videoGroup->selectedId() ||
           NmmKDEConfig::videoHost() != hostlist(m_view->videoListBox);
}

bool NmmConfigDialog::isDefault() const
{
    return false;
}

void NmmConfigDialog::save()
{
    kdDebug() << k_funcinfo << endl;

    if( hasChanged() )
    {
        debug() << "saving changes" << endl;
        NmmKDEConfig::setAudioLocation( m_view->audioGroup->selectedId() );
        NmmKDEConfig::setAudioHost( hostlist(m_view->audioListBox) );
        NmmKDEConfig::setVideoLocation( m_view->videoGroup->selectedId() );
        NmmKDEConfig::setVideoHost( hostlist(m_view->videoListBox) );
        NmmKDEConfig::setAudioOutputPlugin( m_view->audioPlaybackNode->currentText() );
        NmmKDEConfig::writeConfig();
   }
}

void NmmConfigDialog::addAudioHost()
{
    addHost(m_view->audioListBox);
}

void NmmConfigDialog::removeAudioHost()
{
    removeCurrentHost(m_view->audioListBox);
}

void NmmConfigDialog::addVideoHost()
{
    addHost(m_view->videoListBox);
}

void NmmConfigDialog::removeVideoHost()
{
    removeCurrentHost(m_view->videoListBox);
}

void NmmConfigDialog::addHost( QListBox* listBox )
{
    bool ok;
    QString hostname = QInputDialog::getText(
        "New NMM sink host", "Enter hostname to add:", QLineEdit::Normal,
        QString::null, &ok, NULL);
    if( ok && !hostname.isEmpty() )
    {
        listBox->insertItem( hostname );
        listBox->sort();
        emit viewChanged();
    }
}

void NmmConfigDialog::setCheckedAudioList( bool checked )
{
    m_view->audioListBox->setEnabled( checked );
    m_view->addAudioLocationButton->setEnabled( checked );
    m_view->removeAudioLocationButton->setEnabled( checked );
}

void NmmConfigDialog::setCheckedVideoList( bool checked )
{
    m_view->videoListBox->setEnabled( checked );
    m_view->addVideoLocationButton->setEnabled( checked );
    m_view->removeVideoLocationButton->setEnabled( checked );
}

void NmmConfigDialog::removeCurrentHost( QListBox* listBox)
{
    listBox->removeItem( listBox->currentItem() );
    emit viewChanged();
}

QStringList NmmConfigDialog::hostlist( QListBox* listBox) const
{
    QStringList list;
    for (uint i = 0; i < listBox->count(); i++)
        list.append( listBox->text( i ) );
    return list;
}

#include "nmm_configdialog.moc"
