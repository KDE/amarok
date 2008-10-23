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

#include "CollectionConfig.h"

#include "Amarok.h"
#include "CollectionSetup.h"
#include "amarokconfig.h"
#include <config-amarok.h>


CollectionConfig::CollectionConfig( QWidget* parent )
    : ConfigDialogBase( parent )
{
    setupUi( this );

    m_collectionSetup = new CollectionSetup( collectionFoldersBox );
    connect( m_collectionSetup, SIGNAL( changed() ), parent, SLOT( updateButtons() ) );

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget( m_collectionSetup );
    collectionFoldersBox->setLayout( layout );
}

CollectionConfig::~CollectionConfig()
{}


///////////////////////////////////////////////////////////////
// REIMPLEMENTED METHODS from ConfigDialogBase
///////////////////////////////////////////////////////////////

bool
CollectionConfig::hasChanged()
{
    DEBUG_BLOCK

    return m_collectionSetup->hasChanged();
}

bool
CollectionConfig::isDefault()
{
    return false;
}

void
CollectionConfig::updateSettings()
{
    m_collectionSetup->writeConfig();
}

#include "CollectionConfig.moc"


