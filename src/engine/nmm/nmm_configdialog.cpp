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
#include <qwidgetstack.h>
#include <kcombobox.h>

NmmConfigDialog::NmmConfigDialog()
    : PluginConfig(),
    current_audio_group_selection(-1),
    m_environment_list(NULL),
    m_user_list(NULL),
    m_host_list_modified(false)
{
  kdDebug() << k_funcinfo << endl;


  m_view = new NmmConfigDialogBase();

  /* create host list widget stack */
  m_environment_list = new HostList( m_view->audioGroup, "TheEnvironmentList" );
  m_environment_list->setReadOnly( true );
  m_environment_list->setSelectionMode( QListView::NoSelection );

  m_user_list = new HostList( m_view->audioGroup, "TheUserList" );
  m_user_list->setSelectionMode( QListView::Single );
  connect( m_user_list, SIGNAL( viewChanged() ), this, SLOT( hostListModified() ) );
  
  m_view->hostListStack->addWidget( m_environment_list );
  m_view->hostListStack->addWidget( m_user_list );
  // show disabled user list by default if 'localhost only' selected
  m_view->hostListStack->raiseWidget( m_user_list );

  /* restore saved output plugin */
  if ( NmmKDEConfig::audioOutputPlugin() == "ALSAPlaybackNode" )
    m_view->audioPlaybackNode->setCurrentItem( 1 );

  /* restore selected audioGroup selection */
  m_view->audioGroup->setButton( NmmKDEConfig::location() );
  clickedAudioGroup( NmmKDEConfig::location() );
  createHostLists();

  /* connect 'Add...' and 'Remove' buttons */
  connect( m_view->addLocationButton, SIGNAL( released() ), SLOT( addHost() ) );
  connect( m_view->removeHostButton, SIGNAL( released() ), SLOT( removeHost() ) );
  connect( m_user_list, SIGNAL( selectionChanged() ) , this, SLOT( enableRemoveButton() ) );

  /* connect audioGroup selection */
  connect( m_view->audioGroup, SIGNAL( released(int) ), SLOT( clickedAudioGroup(int) ) );

  PluginConfig::connect( m_view->audioPlaybackNode, SIGNAL( activated( int ) ), SIGNAL( viewChanged() ) );
  PluginConfig::connect( m_view->audioGroup, SIGNAL( released( int ) ), SIGNAL( viewChanged() ) );

//  connect( this, SIGNAL( viewChanged() ) )
}

NmmConfigDialog::~NmmConfigDialog()
{
  kdDebug() << k_funcinfo << endl;
  delete m_view;
}


bool NmmConfigDialog::hasChanged() const
{
  return NmmKDEConfig::audioOutputPlugin() != m_view->audioPlaybackNode->currentText() ||
         NmmKDEConfig::location() != m_view->audioGroup->selectedId() ||
         m_host_list_modified;
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
    //NmmEngine::instance()->setEnvironmentHostList( tmp_environment_list );

    /* save user host list and toggle states for audio, video */
    QValueList<NmmLocation> tmp_user_list;

    QListViewItemIterator it( m_user_list );
    HostListItem *host;
    while( it.current() ) {
      host = static_cast<HostListItem*>( it.current() );
      tmp_user_list.append( NmmLocation(host->text(HostListItem::Hostname), host->isAudioEnabled(), host->isVideoEnabled(), /* TODO: host->volume()*/ 0, host->status() ) );
      ++it;
    }

    NmmEngine::instance()->setUserHostList( tmp_user_list );

    QStringList hosts;
    QStringList audio_hosts;
    QStringList video_hosts;
    QValueList<NmmLocation>::iterator it_n;
    for( it_n = tmp_user_list.begin(); it_n != tmp_user_list.end(); ++it_n ) 
    {
      debug() << "saved user host" << endl;
      hosts.append( (*it_n).hostname() );
      if( (*it_n).audio() )
        audio_hosts.append( "1" );
      else 
        audio_hosts.append( "0" );
      if( (*it_n).video() )
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

    m_host_list_modified = false;
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
    new HostListItem( m_user_list, hostname );
    if( m_user_list->currentItem() )
      m_view->removeHostButton->setEnabled( true );
    hostListModified();
  }
}

void NmmConfigDialog::removeHost()
{
    m_user_list->takeItem( m_user_list->currentItem() );
    if( !m_user_list->currentItem() )
      m_view->removeHostButton->setEnabled( false );
    hostListModified();
}

void NmmConfigDialog::clickedAudioGroup( int new_selection )
{
  if( current_audio_group_selection == new_selection || new_selection > 2 )
    return;
    
  /* localhost only, disable host list */
  if( new_selection == 0 ) {
    m_view->hostListStack->setDisabled(true);
    m_view->addLocationButton->setEnabled( false );
    m_view->removeHostButton->setEnabled( false );
  }
  /* environment host list
   * disable 'Add...' and 'Remove' buttons, load environment host list
   */
  else if( new_selection == 1 ) {
    m_view->hostListStack->setEnabled( true );
    m_view->addLocationButton->setEnabled( false );
    m_view->removeHostButton->setEnabled( false );
    m_view->hostListStack->raiseWidget( m_environment_list );
  }
  /* user host list
   * enable all widgets for host list, load user host list
   */
  else if( new_selection == 2 ) {
    m_view->hostListStack->setEnabled( true );
    m_view->addLocationButton->setEnabled( true );
    if( !m_user_list->currentItem() )
      m_view->removeHostButton->setEnabled( false );
    else
      m_view->removeHostButton->setEnabled( true );
    m_view->hostListStack->raiseWidget( m_user_list );
  }

  current_audio_group_selection = new_selection;
}

void NmmConfigDialog::notifyHostError( QString hostname, int error )
{
  m_user_list->notifyHostError( hostname, error );
  m_environment_list->notifyHostError( hostname, error );
}

void NmmConfigDialog::enableRemoveButton()
{

  m_view->removeHostButton->setEnabled( true );
}

void NmmConfigDialog::hostListModified()
{
  m_host_list_modified = true;
  emit viewChanged();
}

void NmmConfigDialog::createHostLists()
{
  DEBUG_BLOCK
    
  QValueList<NmmLocation>::iterator it; 
  QValueList<NmmLocation> list;

  list = NmmEngine::instance()->environmentHostList();
  for( it =  list.begin(); it != list.end(); ++it ) 
    new HostListItem( m_environment_list, (*it).hostname(), (*it).audio(), (*it).video(), 0, (*it).status(), true );

  list = NmmEngine::instance()->userHostList();
  for( it = list.begin(); it != list.end(); ++it ) 
    new HostListItem( m_user_list, (*it).hostname(), (*it).audio(), (*it).video(), 0, (*it).status() );
}

#include "nmm_configdialog.moc"
