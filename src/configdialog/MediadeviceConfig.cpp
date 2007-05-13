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
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "MediadeviceConfig.h"
#include "mediumpluginmanager.h"

#include <Q3GroupBox>

#include <KDialog>
#include <KLocale>
#include <KPushButton>
#include <KVBox>


MediadeviceConfig::MediadeviceConfig( QWidget* parent )
    : ConfigDialogBase( parent )
{
    KVBox* mainWidget = new KVBox( this );
    mainWidget->setSpacing( KDialog::spacingHint() );
    KVBox *topbox = new KVBox( mainWidget );
    topbox->setSpacing( KDialog::spacingHint() );
    Q3GroupBox *mediaBox  = new Q3GroupBox( 2, Qt::Horizontal, i18n("Media Devices"), topbox );
    mediaBox->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Minimum );
    KVBox *vbox = new KVBox( mediaBox );
    vbox->setSpacing( KDialog::spacingHint() );
    m_deviceManager = new MediumPluginManager( vbox );

    KHBox *hbox = new KHBox( topbox );
    hbox->setSpacing( KDialog::spacingHint() );
    hbox->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Minimum );
    KPushButton *autodetect = new KPushButton( i18n( "Autodetect Devices" ), hbox );
    autodetect->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
    connect( autodetect, SIGNAL(clicked()), m_deviceManager, SLOT(redetectDevices()) );
    KPushButton *add = new KPushButton( i18n( "Add Device..." ), hbox );
    add->setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ) );
    connect( add, SIGNAL(clicked()), m_deviceManager, SLOT(newDevice()) );

    QFrame *frame = new QFrame( topbox );
    frame->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
}

MediadeviceConfig::~MediadeviceConfig()
{}


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


