/****************************************************************************************
 * Copyright (c) 2009 John Atkinson <john@fauxnetic.co.uk>                              *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "DatabaseConfig.h"

#include "Amarok.h"
#include "core/support/Debug.h"
#include "CollectionManager.h"

#include <KCMultiDialog>


DatabaseConfig::DatabaseConfig( QWidget* parent )
    : ConfigDialogBase( parent )
{
    setupUi( this );

    // Fix some weird tab orderness
    setTabOrder( kcfg_Host,     kcfg_Port );        // host to port
    setTabOrder( kcfg_Port,     kcfg_User );        // port to username
    setTabOrder( kcfg_User,     kcfg_Password );    // username to password
    setTabOrder( kcfg_Password, kcfg_Database );    // password to database

    connect( kcfg_UseServer, SIGNAL( stateChanged(int) ), SLOT( toggleExternalConfigAvailable(int) ) );

    connect( kcfg_Database, SIGNAL( textChanged(const QString &) ), SLOT( updateSQLQuery() ) );
    connect( kcfg_User,     SIGNAL( textChanged(const QString &) ), SLOT( updateSQLQuery() ) );
    connect( kcfg_Host,     SIGNAL( textChanged(const QString &) ), SLOT( updateSQLQuery() ) );

    toggleExternalConfigAvailable( kcfg_UseServer->checkState() );

    updateSQLQuery();
}

DatabaseConfig::~DatabaseConfig()
{}

void
DatabaseConfig::toggleExternalConfigAvailable( const int checkBoxState ) //SLOT
{
    group_Connection->setEnabled( checkBoxState == Qt::Checked );
}

///////////////////////////////////////////////////////////////
// REIMPLEMENTED METHODS from ConfigDialogBase
///////////////////////////////////////////////////////////////

bool
DatabaseConfig::hasChanged()
{
    return false;
}

bool
DatabaseConfig::isDefault()
{
    return false;
}

void
DatabaseConfig::updateSettings()
{}


///////////////////////////////////////////////////////////////
// PRIVATE METHODS
///////////////////////////////////////////////////////////////

void
DatabaseConfig::updateSQLQuery() //SLOT
{
    QString query;

    if( isSQLInfoPresent() )
    {
        // Query template:
        // GRANT ALL ON amarokdb.* TO 'amarokuser'@'localhost' IDENTIFIED BY 'mypassword'; FLUSH PRIVILEGES;

        // Don't print the actual password!
        const QString examplePassword = i18nc( "A default password for insertion into an example SQL command (so as not to print the real one). To be manually replaced by the user.", "password" );
        query = QString( "CREATE DATABASE %1;\nGRANT ALL PRIVILEGES ON %1.* TO '%2' IDENTIFIED BY '%3'; FLUSH PRIVILEGES;" )
                   .arg( kcfg_Database->text() )
                   .arg( kcfg_User->text() )
                   .arg( examplePassword );
    }
    text_SQL->setPlainText( query );
}


inline bool
DatabaseConfig::isSQLInfoPresent() const
{
    return !kcfg_Database->text().isEmpty() && !kcfg_User->text().isEmpty() && !kcfg_Host->text().isEmpty();
}


#include "DatabaseConfig.moc"


