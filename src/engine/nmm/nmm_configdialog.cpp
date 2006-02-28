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

  m_hosts.setAutoDelete( true );

  /* populate envionment host list */
  createEnvironmentHostList();
  createUserHostList();

  m_view = new NmmConfigDialogBase();

  /* restore saved output plugin */
  if ( NmmKDEConfig::audioOutputPlugin() == "ALSAPlaybackNode" )
    m_view->audioPlaybackNode->setCurrentItem( 1 );

  audio_vbox = new QWidget(m_view->locationScrollView->viewport());
  audio_vbox->setPaletteBackgroundColor(Qt::white);

  QVBoxLayout *audio_vbox_layout = new QVBoxLayout(audio_vbox);
  audio_vbox_layout->setAlignment( Qt::AlignAuto | Qt::AlignTop );
  audio_vbox_layout->setAutoAdd( true );

  m_view->locationScrollView->addChild(audio_vbox);
  m_view->locationScrollView->setResizePolicy(QScrollView::AutoOneFit);
  //m_view->locationScrollView->setHScrollBarMode( QScrollView::AlwaysOff );

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
  m_hosts.setAutoDelete( false );
  delete m_view;
}


bool NmmConfigDialog::hasChanged() const
{
  // TODO
  return NmmKDEConfig::audioOutputPlugin() != m_view->audioPlaybackNode->currentText() ||
    NmmKDEConfig::location() != m_view->audioGroup->selectedId() ||
    NmmKDEConfig::hostList() != hostList();
  //         NmmKDEConfig::audioHost() != audioHostList() ||
  //         NmmKDEConfig::videoHost() != videoHostList();
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
    QStringList hosts;
    QStringList audio_hosts;
    QStringList video_hosts;
    NmmLocationList::iterator it;
    for( it = tmp_user_list.begin(); it != tmp_user_list.end(); ++it ){
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
    //audio_vbox->layout()->invalidate();
    //m_view->audioScrollView->update();
    emit viewChanged();
  }
}

void NmmConfigDialog::removeHost()
{
  if( current_host ) {
    m_hosts.remove( current_host );
    //delete current_host; // auto delete
    current_host = NULL;

    // no item selected, disable remove button
    m_view->removeLocationButton->setEnabled( false );
  }
}

void NmmConfigDialog::addHostListItem( QString hostname, bool audio, bool video, int volume, bool read_only )
{
  HostListItem *item = new HostListItem( audio_vbox, hostname, audio, video, volume );

  if( !read_only ) {
    connect( item, SIGNAL( clicked( HostListItem* ) ), this, SLOT( selectHostListItem( HostListItem* ) ) );
    // TODO: connect audio,video toggle to viewChanged()
  }

  item->show();
  m_hosts.append( item );
}

void NmmConfigDialog::createEnvironmentHostList()
{
  tmp_environment_list = NmmEngine::instance()->environmentHostList();
}

void NmmConfigDialog::createUserHostList()
{
  DEBUG_BLOCK

  QStringList hosts = NmmKDEConfig::hostList();
  QStringList audio_list = NmmKDEConfig::audioToggle();
  QStringList video_list = NmmKDEConfig::videoToggle();

  bool audio = false;
  bool video = false;

  unsigned int size = hosts.size();
  for(unsigned int i = 0; i < size; i++ ) {
    if( audio_list[i] == "1")
      audio = true;
    else 
      audio = false;
    
    if( video_list[i] == "1")
      video = true;
    else 
      video = false;
   
    tmp_user_list.append( NmmLocation( hosts[i], audio, video, /* TODO: volume */0) );
  }
}

void NmmConfigDialog::createHostList( bool use_environment_list )
{
  DEBUG_BLOCK
  
  NmmLocationList::iterator it; 

  if( use_environment_list )
    for( it =  tmp_environment_list.begin(); it != tmp_environment_list.end(); ++it ) 
      addHostListItem( (*it).hostname(), (*it).audio(), (*it).video(), 0 );

  else
    for( it = tmp_user_list.begin(); it != tmp_user_list.end(); ++it ) 
      addHostListItem( (*it).hostname(), (*it).audio(), (*it).video(), 0 );
}

QStringList NmmConfigDialog::hostList() const
{
  QStringList list;
  QPtrListIterator<HostListItem> it( m_hosts );
  HostListItem *hosts;
  while ( (hosts = it.current()) != 0 ) {
    ++it;
    list.append( hosts->hostname() );
  }
  return list;
}

QStringList NmmConfigDialog::audioHostList() const
{
  QStringList list;
  QPtrListIterator<HostListItem> it( m_hosts );
  HostListItem *hosts;
  while ( (hosts = it.current()) != 0 ) {
    ++it;
    if( hosts->isAudioEnabled() )
      list.append( hosts->hostname() );
  }

  return list;
}

QStringList NmmConfigDialog::videoHostList() const
{
  QStringList list;
  QPtrListIterator<HostListItem> it( m_hosts );
  HostListItem *hosts;
  while ( (hosts = it.current()) != 0 ) {
    ++it;
    if( hosts->isVideoEnabled() )
      list.append( hosts->hostname() );
  }

  return list;
}

void NmmConfigDialog::removeHostList( bool save_user_hostlist )
{
  if( save_user_hostlist )
    saveUserHostList();

  // remove all host items
  m_hosts.clear();
  current_host = NULL;
}

void NmmConfigDialog::saveUserHostList()
{
  tmp_user_list.clear();
  QPtrListIterator<HostListItem> it( m_hosts );
  HostListItem *host;
  while ( (host = it.current()) != 0 ) {
    ++it;
    tmp_user_list.append( NmmLocation(host->hostname(), host->isAudioEnabled(), host->isVideoEnabled(), /* TODO: host->volume()*/ 0 ) );
  }
}

void NmmConfigDialog::selectHostListItem( HostListItem *item )
{
  if(current_host)
    current_host->setHighlighted( false );

  current_host = item;
  current_host->setHighlighted( true );

  m_view->removeLocationButton->setEnabled( true );
}

void NmmConfigDialog::clickedAudioGroup( int s )
{
  DEBUG_BLOCK

  static int _s = -1;
  if( _s == s )
    return;
    
  // localhost only, disable host list
  if( s == 0 ) {
    m_view->locationScrollView->setEnabled( false );
    m_view->addLocationButton->setEnabled( false );
    m_view->removeLocationButton->setEnabled( false );
  }
  // environment host list
  // disable 'Add...' and 'Remove' buttons, load environment host list
  else if( s == 1 ) {
    m_view->locationScrollView->setEnabled( true );
    m_view->addLocationButton->setEnabled( false );
    m_view->removeLocationButton->setEnabled( false );

    removeHostList( _s == 2 ? true : false );
    createHostList( true );
  }
  // user host list
  // enable all widgets for host list, load user host list
  else {
    m_view->locationScrollView->setEnabled( true );
    m_view->addLocationButton->setEnabled( true );
    m_view->removeLocationButton->setEnabled( true );

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
