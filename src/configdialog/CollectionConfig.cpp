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

#include "CollectionConfig.h"
#include "amarok.h"
#include "amarokconfig.h"
#include "config-amarok.h"
#include "directorylist.h"


CollectionConfig::CollectionConfig( QWidget* parent )
    : ConfigDialogBase( parent )
{
    setupUi( this ); 

#if !defined(USE_MYSQL) && !defined(USE_POSTGRESQL)
    databaseBox->hide();
#endif

#ifndef USE_MYSQL
    //FIXME we do this because this widget breaks the Apply button (always enabled).
    //It breaks because it is set to type="password" in the .kcfg file. Setting to
    //type="string" also fixes this bug, but means the password is stored in plain
    //text. This is a temporary fix so that the majority of users get a fixed Apply
    //button.
    delete dbSetupFrame->kcfg_MySqlPassword2;
#endif
    collectionFoldersBox->setColumns( 1 );
    new CollectionSetup( collectionFoldersBox ); //TODO this widget doesn't update the apply/ok buttons

    connect( dbSetupFrame->databaseEngine, SIGNAL( activated( int ) ), parent, SLOT( updateButtons() ) );
}

CollectionConfig::~CollectionConfig()
{}


///////////////////////////////////////////////////////////////
// REIMPLEMENTED METHODS from ConfigDialogBase
///////////////////////////////////////////////////////////////

bool
CollectionConfig::hasChanged()
{
    return Amarok::databaseTypeCode( dbSetupFrame->databaseEngine->currentText() ) != AmarokConfig::databaseEngine().toInt();
}

bool
CollectionConfig::isDefault()
{
    return false;
}

void
CollectionConfig::updateSettings()
{
    const int dbType = Amarok::databaseTypeCode( dbSetupFrame->databaseEngine->currentText() );
    if ( dbType != AmarokConfig::databaseEngine().toInt() ) {
        AmarokConfig::setDatabaseEngine( QString::number( dbType ) );
        emit settingsChanged( parent()->objectName() );
    }

}


///////////////////////////////////////////////////////////////
// PRIVATE METHODS 
///////////////////////////////////////////////////////////////


#include "CollectionConfig.moc"


