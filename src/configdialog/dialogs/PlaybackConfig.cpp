/***************************************************************************
 *   Copyright (C) 2004-2009 by Mark Kretschmann <kretschmann@kde.org>     *
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

#include "PlaybackConfig.h"

#include "Debug.h"

#include <KCMultiDialog>


PlaybackConfig::PlaybackConfig( QWidget* parent )
    : ConfigDialogBase( parent )
{
    setupUi( this );
    kcfg_FadeoutOnExit->setHidden( true );

    connect( findChild<QPushButton*>( "pushButtonPhonon" ), SIGNAL( clicked() ), SLOT( configurePhonon() ) );
}

PlaybackConfig::~PlaybackConfig()
{}


///////////////////////////////////////////////////////////////
// REIMPLEMENTED METHODS from ConfigDialogBase
///////////////////////////////////////////////////////////////

bool
PlaybackConfig::hasChanged()
{
    return false;
}

bool
PlaybackConfig::isDefault()
{
    return false;
}

void
PlaybackConfig::updateSettings()
{}


///////////////////////////////////////////////////////////////
// PRIVATE METHODS 
///////////////////////////////////////////////////////////////

void
PlaybackConfig::configurePhonon() //SLOT
{
    DEBUG_BLOCK

    KCMultiDialog* KCM = new KCMultiDialog();
    KCM->setWindowTitle( i18n( "Configure Phonon - Amarok" ) );
    KCM->addModule( "kcm_phonon" );
    KCM->exec();

    KCM->deleteLater();
}


#include "PlaybackConfig.moc"


