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
    : PluginConfig()
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
  connect( m_view->removeLocationButton, SIGNAL( released() ), SLOT( removeHost() ) );

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
  return NmmKDEConfig::audioOutputPlugin() != m_view->audioPlaybackNode->currentText() ||
    NmmKDEConfig::location() != m_view->audioGroup->selectedId();
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

    NmmKDEConfig::setAudioOutputPlugin( m_view->audioPlaybackNode->currentText() );
    NmmKDEConfig::setLocation( m_view->audioGroup->selectedId() );

    /* store volume for AUDIO_HOSTS */
    NmmEngine::instance()->setEnvironmentHostList( tmp_environment_list );

    /* save user host list and toggle sattes for audio, video */
    saveUserHostList();
    NmmEngine::instance()->setUserHostList( tmp_user_list );

    QStringList hosts;
    QStringList audio_hosts;
    QStringList video_hosts;
    QListViewItemIterator it( m_view->hostList );
    HostListItem *host;
    while( it.current() )
    {
      host = static_cast<HostListItem*>( it.current() );
      hosts.append( host->text( HostListItem::Hostname ) );
      if( host->isAudioEnabled() )
        audio_hosts.append( "1" );
      else 
        audio_hosts.append( "0" );
      if( host->isVideoEnabled() )
        video_hosts.append( "1" );
      else
        video_hosts.append( "0" );
      // TODO: save volume

      ++it;
    }
    NmmKDEConfig::setHostList( hosts );
    NmmKDEConfig::setAudioToggle( audio_hosts );
    NmmKDEConfig::setVideoToggle( video_hosts );

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
//  if( current_host ) {
//    m_hosts.remove( current_host );
//    //delete current_host; // auto delete
//    current_host = NULL;
//
//    // no item selected, disable remove button
//    m_view->removeLocationButton->setEnabled( false );
//  }
}

void NmmConfigDialog::addHostListItem( QString hostname, bool audio, bool video, int volume, bool read_only )
{
  HostListItem *item = new HostListItem( m_view->hostList, hostname, audio, video, volume, read_only );
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

void NmmConfigDialog::clickedAudioGroup( int s )
{
  DEBUG_BLOCK

  static int _s = -1;
  if( _s == s || s > 2 )
    return;
    
  // localhost only, disable host list
  if( s == 0 ) {
    m_view->hostList->setEnabled( false );
    m_view->addLocationButton->setEnabled( false );
    m_view->removeLocationButton->setEnabled( false );
  }
  // environment host list
  // disable 'Add...' and 'Remove' buttons, load environment host list
  else if( s == 1 ) {
    m_view->hostList->setEnabled( true );
    m_view->addLocationButton->setEnabled( false );
    m_view->removeLocationButton->setEnabled( false );

    removeHostList( _s == 2 ? true : false );
    createHostList( true );
  }
  // user host list
  // enable all widgets for host list, load user host list
  else if( s == 2 ){
    m_view->hostList->setEnabled( true );
    m_view->addLocationButton->setEnabled( true );
    m_view->removeLocationButton->setEnabled( false );

    removeHostList();
    createHostList();
  }

  _s = s;
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
