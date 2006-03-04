/* NMM - Network-Integrated Multimedia Middleware
 *
 * Copyright (C) 2005-2006
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
#include "nmm_engine.h"
#include "nmm_kdeconfig.h"
#include "HostList.h"
#include "HostListItem.h"
#include "debug.h"

#include <qbuttongroup.h>
#include <qinputdialog.h>
#include <qlayout.h>
#include <qlistbox.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <kcombobox.h>

NmmConfigDialog::NmmConfigDialog()
    : PluginConfig(),
    current_audio_group_selection(-1)
{
  kdDebug() << k_funcinfo << endl;

  /* populate envionment and user host list */
  tmp_environment_list = NmmEngine::instance()->environmentHostList();
  tmp_user_list = NmmEngine::instance()->userHostList();

  m_view = new NmmConfigDialogBase();

  /* restore saved output plugin */
  if ( NmmKDEConfig::audioOutputPlugin() == "ALSAPlaybackNode" )
    m_view->audioPlaybackNode->setCurrentItem( 1 );

  m_view->audioGroup->setButton( NmmKDEConfig::location() );
  clickedAudioGroup( NmmKDEConfig::location() );

  /* connect 'Add...' and 'Remove' buttons */
  connect( m_view->addLocationButton, SIGNAL( released() ), SLOT( addHost() ) );
  connect( m_view->removeHostButton, SIGNAL( released() ), SLOT( removeHost() ) );
  connect( m_view->hostList, SIGNAL( selectionChanged() ) , this, SLOT( enableRemoveButton() ) );

  /* connect audioGroup selection */
  connect( m_view->audioGroup, SIGNAL( released(int) ), SLOT( clickedAudioGroup(int) ) );

  PluginConfig::connect( m_view->audioPlaybackNode, SIGNAL( activated( int ) ), SIGNAL( viewChanged() ) );
  PluginConfig::connect( m_view->audioGroup, SIGNAL( released( int ) ), SIGNAL( viewChanged() ) );
}

NmmConfigDialog::~NmmConfigDialog()
{
  kdDebug() << k_funcinfo << endl;
  delete m_view;
}


bool NmmConfigDialog::hasChanged() const
{
  // TODO: observe user list changes
  return true; //NmmKDEConfig::audioOutputPlugin() != m_view->audioPlaybackNode->currentText() ||
//    NmmKDEConfig::location() != m_view->audioGroup->selectedId();
}

bool NmmConfigDialog::isDefault() const
{
    return false;
}

void NmmConfigDialog::save()
{
  DEBUG_BLOCK

  if( hasChanged() )
  {
    NmmKDEConfig::setAudioOutputPlugin( m_view->audioPlaybackNode->currentText() );
    debug() << "saved audio output plugin" << endl;
    NmmKDEConfig::setLocation( m_view->audioGroup->selectedId() );
    debug() << "saved current location selection" << endl;

    /* store volume for AUDIO_HOSTS */
    NmmEngine::instance()->setEnvironmentHostList( tmp_environment_list );

    /* save user host list and toggle states for audio, video */
    if( m_view->hostListButton->isChecked() )
      saveUserHostList();
    NmmEngine::instance()->setUserHostList( tmp_user_list );

    QStringList hosts;
    QStringList audio_hosts;
    QStringList video_hosts;
    NmmLocationList::iterator it; 
    for( it = tmp_user_list.begin(); it != tmp_user_list.end(); ++it ) 
    {
      debug() << "saved user host" << endl;
      hosts.append( (*it).hostname() );
      if( (*it).audio() )
        audio_hosts.append( "1" );
      else 
        audio_hosts.append( "0" );
      if( (*it).video() )
        video_hosts.append( "1" );
      else
        video_hosts.append( "0" );
      // TODO: save volume
    }
    NmmKDEConfig::setHostList( hosts );
    NmmKDEConfig::setAudioToggle( audio_hosts );
    NmmKDEConfig::setVideoToggle( video_hosts );
    debug() << "saved user host list with toggle states for audio and video" << endl;

    NmmKDEConfig::writeConfig();
  }
}

void NmmConfigDialog::addHost()
{
  bool ok;
  QString hostname = QInputDialog::getText(
      "New NMM sink host", "Enter hostname to add:", QLineEdit::Normal,
      QString::null, &ok, NULL);
  if( ok && !hostname.isEmpty() )
  {
    addHostListItem( hostname );
    emit viewChanged();
  }
}

void NmmConfigDialog::removeHost()
{
    m_view->hostList->takeItem( m_view->hostList->currentItem() );

    // no item selected, disable remove button
    m_view->removeHostButton->setEnabled( false );
}

void NmmConfigDialog::addHostListItem( QString hostname, bool audio, bool video, int volume, bool read_only )
{
  new HostListItem( m_view->hostList, hostname, audio, video, volume, read_only );
}

void NmmConfigDialog::createHostList( bool use_environment_list )
{
  DEBUG_BLOCK
  
  NmmLocationList::iterator it; 

  if( use_environment_list )
    for( it =  tmp_environment_list.begin(); it != tmp_environment_list.end(); ++it ) 
      addHostListItem( (*it).hostname(), (*it).audio(), (*it).video(), 0, true);

  else
    for( it = tmp_user_list.begin(); it != tmp_user_list.end(); ++it ) 
      addHostListItem( (*it).hostname(), (*it).audio(), (*it).video(), 0 );
}

void NmmConfigDialog::removeHostList( bool save_user_hostlist )
{
  if( save_user_hostlist )
    saveUserHostList();

  m_view->hostList->clear();
}

void NmmConfigDialog::saveUserHostList()
{
  tmp_user_list.clear();
  QListViewItemIterator it( m_view->hostList );
  HostListItem *host;
  while( it.current() ) {
    host = static_cast<HostListItem*>( it.current() );
    tmp_user_list.append( NmmLocation(host->text(HostListItem::Hostname), host->isAudioEnabled(), host->isVideoEnabled(), /* TODO: host->volume()*/ 0 ) );
    ++it;
  }
}

void NmmConfigDialog::clickedAudioGroup( int new_selection )
{
  DEBUG_BLOCK

  if( current_audio_group_selection == new_selection || new_selection > 2 )
    return;
    
  // localhost only, disable host list
  if( new_selection == 0 ) {
    m_view->hostList->setEnabled( false );
    m_view->addLocationButton->setEnabled( false );
    m_view->removeHostButton->setEnabled( false );
  }
  // environment host list
  // disable 'Add...' and 'Remove' buttons, load environment host list
  else if( new_selection == 1 ) {
    m_view->hostList->setEnabled( true );
    m_view->addLocationButton->setEnabled( false );
    m_view->removeHostButton->setEnabled( false );
    m_view->hostList->setSelectionMode( QListView::NoSelection );

    removeHostList( current_audio_group_selection == 2 ? true : false );
    createHostList( true );
  }
  // user host list
  // enable all widgets for host list, load user host list
  else if( new_selection == 2 ) {
    m_view->hostList->setEnabled( true );
    m_view->addLocationButton->setEnabled( true );
    m_view->removeHostButton->setEnabled( false );
    m_view->hostList->setSelectionMode( QListView::Single );

    removeHostList();
    createHostList();
  }

  current_audio_group_selection = new_selection;
}

void NmmConfigDialog::enableRemoveButton()
{
  // m_view->hostList->setCurrentItem( m_view->hostList->selectedItem() );
  m_view->removeHostButton->setEnabled( true );
}

// ###> NmmLocation class ### {{{
NmmLocation::NmmLocation()
{}

NmmLocation::NmmLocation(QString hostname, bool audio, bool video, int volume)
  : m_hostname(hostname), m_audio(audio), m_video(video), m_volume(volume)
{
}

NmmLocation::~NmmLocation()
{}

QString NmmLocation::hostname() const
{
  return m_hostname;
}

void NmmLocation::setHostname(QString hostname)
{
  m_hostname = hostname;
}
// ###< NmmLocation class ### }}}

#include "nmm_configdialog.moc"
