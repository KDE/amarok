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

    m_view = new NmmConfigDialogBase();

    /* restored saved output plugin */
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
    
    readConfig();

    connect( m_view->addLocationButton, SIGNAL( released() ), SLOT( addHost() ) );
    connect( m_view->removeLocationButton, SIGNAL( released() ), SLOT( removeHost() ) );
    connect( m_view->hostListButton, SIGNAL( toggled(bool) ), SLOT( setCheckedList(bool) ) );

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
        NmmKDEConfig::setHostList( hostList() );
        // TODO: save toggle states
        //NmmKDEConfig::setAudioHost( audioHostList() );
        //NmmKDEConfig::setVideoHost( videoHostList() );
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

        delete current_host;
        current_host = NULL;

        // no item selected, disable remove button
        m_view->removeLocationButton->setEnabled( false );
    }
}

void NmmConfigDialog::setCheckedList( bool checked )
{
    m_view->locationScrollView->setEnabled( checked );
    m_view->addLocationButton->setEnabled( checked );
    if( checked && current_host )
        m_view->removeLocationButton->setEnabled( true );
    else 
        m_view->removeLocationButton->setEnabled( false );
}

void NmmConfigDialog::readConfig()
{
    m_view->audioGroup->setButton( NmmKDEConfig::location() );
    setCheckedList( m_view->hostListButton->isChecked() );

    QStringList hosts = NmmKDEConfig::hostList();
    // TODO: read audio/video toggle states
    QStringList::iterator it;
    for ( it = hosts.begin(); it != hosts.end(); ++it )
        addHostListItem( *it );
    
}

void NmmConfigDialog::addHostListItem( QString hostname )
{
    HostListItem *item = new HostListItem(false, hostname, audio_vbox);
    connect( item, SIGNAL( pressed( HostListItem* ) ), this, SLOT( selectHostListItem( HostListItem* ) ) );
    // TODO: connect audio,video toggle to viewChanged()
    item->show();
    m_hosts.append( item );
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
  // TODO
  return QStringList();
}

QStringList NmmConfigDialog::videoHostList() const
{
  // TODO
  return QStringList();
}

void NmmConfigDialog::selectHostListItem( HostListItem *item )
{
    if(current_host)
        current_host->setHighlighted( false );

    current_host = item;
    current_host->setHighlighted( true );

    m_view->removeLocationButton->setEnabled( true );
}

#include "nmm_configdialog.moc"
