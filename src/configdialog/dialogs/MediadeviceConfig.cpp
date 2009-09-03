/****************************************************************************************
 * Copyright (c) 2004-2007 Mark Kretschmann <kretschmann@kde.org>                       *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "MediadeviceConfig.h"
#include "MediaDevicePluginManager.h"

#include <QGroupBox>
#include <QVBoxLayout>

#include <KDialog>
#include <KLocale>
#include <KPushButton>
#include <KVBox>


MediadeviceConfig::MediadeviceConfig( QWidget* parent )
    : ConfigDialogBase( parent )
    , m_genericDevices( 0 )
    , m_add( 0 )
    , m_pluginManager( 0 )
{
    QVBoxLayout* mainLayout = new QVBoxLayout( this );
    mainLayout->setSpacing( KDialog::spacingHint() );
    KVBox *topbox = new KVBox( this );
    mainLayout->addWidget( topbox );
    topbox->setSpacing( KDialog::spacingHint() );
    QGroupBox *mediaBox  = new QGroupBox( i18n("Media Devices"), topbox );
    mediaBox->setAlignment( Qt::Horizontal );
    mediaBox->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Minimum );
    KVBox *vbox = new KVBox( mediaBox );
    vbox->setSpacing( KDialog::spacingHint() );
    m_pluginManager = new MediaDevicePluginManager( vbox );

    KHBox *hbox = new KHBox( topbox );
    hbox->setSpacing( KDialog::spacingHint() );
    hbox->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Minimum );
    m_genericDevices = new KPushButton( i18n( "Generic Devices and Volumes..." ), hbox );
    m_genericDevices->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
    connect( m_genericDevices, SIGNAL(clicked()), m_pluginManager, SLOT(slotGenericVolumes()) );
    m_add = new KPushButton( i18n( "Add Device..." ), hbox );
    m_add->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
    connect( m_add, SIGNAL(clicked()), m_pluginManager, SLOT(slotNewDevice()) );

    QFrame *frame = new QFrame( topbox );
    frame->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );

}

MediadeviceConfig::~MediadeviceConfig()
{
    disconnect( m_genericDevices, SIGNAL(clicked()), m_pluginManager, SLOT(slotGenericVolumes()) );
    disconnect( m_add, SIGNAL(clicked()), m_pluginManager, SLOT(slotNewDevice()) );
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
    if( m_pluginManager )
        m_pluginManager->finished();
}


///////////////////////////////////////////////////////////////
// PRIVATE METHODS 
///////////////////////////////////////////////////////////////


#include "MediadeviceConfig.moc"


