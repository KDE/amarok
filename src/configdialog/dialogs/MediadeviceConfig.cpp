/***************************************************************************
 *   Copyright (C) 2004-2007 by Mark Kretschmann <markey@web.de>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "MediadeviceConfig.h"
#include "MediaDevicePluginManager.h"

#include <Q3GroupBox>
#include <QVBoxLayout>

#include <KDialog>
#include <KLocale>
#include <KPushButton>
#include <KVBox>


MediadeviceConfig::MediadeviceConfig( QWidget* parent )
    : ConfigDialogBase( parent )
    , m_autodetect( 0 )
    , m_add( 0 )
    , m_pluginManager( 0 )
{
    QVBoxLayout* mainLayout = new QVBoxLayout( this );
    mainLayout->setSpacing( KDialog::spacingHint() );
    KVBox *topbox = new KVBox( this );
    mainLayout->addWidget( topbox );
    topbox->setSpacing( KDialog::spacingHint() );
    Q3GroupBox *mediaBox  = new Q3GroupBox( 2, Qt::Horizontal, i18n("Media Devices"), topbox );
    mediaBox->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Minimum );
    KVBox *vbox = new KVBox( mediaBox );
    vbox->setSpacing( KDialog::spacingHint() );
    m_pluginManager = new MediaDevicePluginManager( vbox );

    KHBox *hbox = new KHBox( topbox );
    hbox->setSpacing( KDialog::spacingHint() );
    hbox->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Minimum );
    m_autodetect = new KPushButton( i18n( "Autodetect Devices" ), hbox );
    m_autodetect->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
    connect( m_autodetect, SIGNAL(clicked()), m_pluginManager, SLOT(redetectDevices()) );
    m_add = new KPushButton( i18n( "Add Device..." ), hbox );
    m_add->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
    connect( m_add, SIGNAL(clicked()), m_pluginManager, SLOT(newDevice()) );

    QFrame *frame = new QFrame( topbox );
    frame->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

}

MediadeviceConfig::~MediadeviceConfig()
{
    disconnect( m_autodetect, SIGNAL(clicked()), m_pluginManager, SLOT(redetectDevices()) );        
    disconnect( m_add, SIGNAL(clicked()), m_pluginManager, SLOT(newDevice()) );       
    m_pluginManager->finished();
    delete m_pluginManager;
}


///////////////////////////////////////////////////////////////
// REIMPLEMENTED METHODS from ConfigDialogBase
///////////////////////////////////////////////////////////////

bool
MediadeviceConfig::hasChanged()
{
    return false;
}

bool
MediadeviceConfig::isDefault()
{
    return false;
}

void
MediadeviceConfig::updateSettings()
{
}


///////////////////////////////////////////////////////////////
// PRIVATE METHODS 
///////////////////////////////////////////////////////////////


#include "MediadeviceConfig.moc"


