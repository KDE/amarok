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

#include "CollectionConfig.h"

#include "amarokconfig.h"
#include <config.h>
#include "configdialog/ConfigDialog.h"
#include "core/support/Amarok.h"
#include "core-impl/collections/db/sql/SqlCollection.h"
#include "dialogs/CollectionSetup.h"

CollectionConfig::CollectionConfig( Amarok2ConfigDialog* parent )
    : ConfigDialogBase( parent )
{
    m_collectionSetup = new CollectionSetup( this );

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget( m_collectionSetup );
    setLayout( layout );

    KConfigGroup transcodeGroup = Amarok::config( Collections::SQL_TRANSCODING_GROUP_NAME );
    m_collectionSetup->transcodingConfig()->fillInChoices( Transcoding::Configuration::fromConfigGroup( transcodeGroup ) );


    connect( m_collectionSetup, &CollectionSetup::changed, parent, &Amarok2ConfigDialog::updateButtons );
    connect( m_collectionSetup->transcodingConfig(), QOverload<int>::of(&QComboBox::currentIndexChanged),
             parent, &Amarok2ConfigDialog::updateButtons );
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

    return m_collectionSetup->hasChanged() || m_collectionSetup->transcodingConfig()->hasChanged();
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

    KConfigGroup transcodeGroup = Amarok::config( Collections::SQL_TRANSCODING_GROUP_NAME );
    m_collectionSetup->transcodingConfig()->currentChoice().saveToConfigGroup( transcodeGroup );
}

