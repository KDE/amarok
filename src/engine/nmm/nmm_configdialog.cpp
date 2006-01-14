/* NMM - Network-Integrated Multimedia Middleware
 *
 * Copyright (C) 2005
 *                    NMM work group,
 *                    Computer Graphics Lab,
 *                    Saarland University, Germany
 *                    http://www.networkmultimedia.org
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02111-1307
 * USA
 */

#include "nmm_configdialog.h"
#include "nmm_kdeconfig.h"
#include "AudioHostListItem.h"
#include "debug.h"

#include <qbuttongroup.h>
#include <qinputdialog.h>
#include <qlayout.h>
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

    audio_vbox = new QWidget(m_view->audioScrollView->viewport());
    audio_vbox->setPaletteBackgroundColor(Qt::white);

    QVBoxLayout *audio_vbox_layout = new QVBoxLayout(audio_vbox);
    audio_vbox_layout->setAlignment( Qt::AlignAuto | Qt::AlignTop );
    audio_vbox_layout->setAutoAdd( true );
    
    m_view->audioScrollView->addChild(audio_vbox);
    m_view->audioScrollView->setResizePolicy(QScrollView::AutoOneFit);
    m_view->audioScrollView->setHScrollBarMode( QScrollView::AlwaysOff );
    
    readConfig();

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
           NmmKDEConfig::audioHost() != audioHostList() ||
           NmmKDEConfig::videoLocation() != m_view->videoGroup->selectedId() ||
           NmmKDEConfig::videoHost() != videoHostList();
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
        NmmKDEConfig::setAudioHost( audioHostList() );
        NmmKDEConfig::setVideoLocation( m_view->videoGroup->selectedId() );
        NmmKDEConfig::setVideoHost( videoHostList() );
        NmmKDEConfig::setAudioOutputPlugin( m_view->audioPlaybackNode->currentText() );
        NmmKDEConfig::writeConfig();
   }
}

void NmmConfigDialog::addAudioHost()
{
    bool ok;
    QString hostname = QInputDialog::getText(
        "New NMM sink host", "Enter hostname to add:", QLineEdit::Normal,
        QString::null, &ok, NULL);
    if( ok && !hostname.isEmpty() )
    {
        addAudioHostListItem( hostname );
        //audio_vbox->layout()->invalidate();
        //m_view->audioScrollView->update();
        emit viewChanged();
    }
}

void NmmConfigDialog::removeAudioHost()
{
    if( current_audio_host ) {
        m_audio_hosts.remove( current_audio_host );

        delete current_audio_host;
        current_audio_host = NULL;

        // no item selected, disable remove button
        m_view->removeAudioLocationButton->setEnabled( false );
    }
}

void NmmConfigDialog::addVideoHost()
{
    bool ok;
    QString hostname = QInputDialog::getText(
        "New NMM sink host", "Enter hostname to add:", QLineEdit::Normal,
        QString::null, &ok, NULL);
    if( ok && !hostname.isEmpty() ) // TODO: check for duplicates!?
    {
        m_view->videoListBox->insertItem( hostname );
        m_view->videoListBox->sort();
        emit viewChanged();
    }
}

void NmmConfigDialog::removeVideoHost()
{
    m_view->videoListBox->removeItem( m_view->videoListBox->currentItem() );
    emit viewChanged();
}

void NmmConfigDialog::setCheckedAudioList( bool checked )
{
    m_view->audioScrollView->setEnabled( checked );
    m_view->addAudioLocationButton->setEnabled( checked );
    if( checked && current_audio_host )
        m_view->removeAudioLocationButton->setEnabled( true );
    else 
        m_view->removeAudioLocationButton->setEnabled( false );
}

void NmmConfigDialog::setCheckedVideoList( bool checked )
{
    m_view->videoListBox->setEnabled( checked );
    m_view->addVideoLocationButton->setEnabled( checked );
    m_view->removeVideoLocationButton->setEnabled( checked );
}

void NmmConfigDialog::readConfig()
{
    m_view->audioGroup->setButton( NmmKDEConfig::audioLocation() );
    setCheckedAudioList( m_view->audioHostListButton->isChecked() );

    QStringList audioHosts = NmmKDEConfig::audioHost();
    QStringList::iterator it;
    for ( it = audioHosts.begin(); it != audioHosts.end(); ++it )
        addAudioHostListItem( *it );
    
    m_view->videoGroup->setButton( NmmKDEConfig::videoLocation() );
    setCheckedVideoList( m_view->videoHostListButton->isChecked() );
    m_view->videoListBox->insertStringList( NmmKDEConfig::videoHost() );
}

void NmmConfigDialog::addAudioHostListItem( QString hostname )
{
    AudioHostListItem *item = new AudioHostListItem(false, hostname, audio_vbox);
    connect( item, SIGNAL( pressed( AudioHostListItem* ) ), this, SLOT( selectHostListItem( AudioHostListItem* ) ) );
    item->show();
    m_audio_hosts.append( item );

}

QStringList NmmConfigDialog::audioHostList() const
{
    QStringList list;
    QPtrListIterator<AudioHostListItem> it( m_audio_hosts );
    AudioHostListItem *audioHost;
    while ( (audioHost = it.current()) != 0 ) {
        ++it;
        list.append( audioHost->hostname() );
    }
    return list;
}

QStringList NmmConfigDialog::videoHostList() const
{
    QStringList list;
    for (uint i = 0; i < m_view->videoListBox->count(); i++)
        list.append( m_view->videoListBox->text( i ) );
    return list;
}

void NmmConfigDialog::selectHostListItem( AudioHostListItem *item )
{
    if(current_audio_host)
        current_audio_host->setHighlighted( false );

    current_audio_host = item;
    current_audio_host->setHighlighted( true );

    m_view->removeAudioLocationButton->setEnabled( true );
}

#include "nmm_configdialog.moc"
