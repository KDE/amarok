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
#include "Debug.h"

#include <KCMultiDialog>


DatabaseConfig::DatabaseConfig( QWidget* parent )
    : ConfigDialogBase( parent )
{
    setupUi( this );

    // Fix some weird tab orderness
    setTabOrder( kcfg_Server,   kcfg_Port );     // server to port
    setTabOrder( kcfg_Port,     kcfg_Username ); // port to username
    setTabOrder( kcfg_Username, kcfg_Password ); // username to password
    setTabOrder( kcfg_Password, kcfg_DBName );   // password to database

    readConfiguration();

    connect( kcfg_UseInternalDB, SIGNAL( stateChanged(int) ), SLOT( toggleExternalConfigAvailable(int) ) );

    connect( kcfg_DBName,   SIGNAL( textChanged(const QString &) ), SLOT( updateSQLQuery() ) );
    connect( kcfg_Username, SIGNAL( textChanged(const QString &) ), SLOT( updateSQLQuery() ) );
    connect( kcfg_Server,   SIGNAL( textChanged(const QString &) ), SLOT( updateSQLQuery() ) );

}

DatabaseConfig::~DatabaseConfig()
{}


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
{
    writeConfiguration();
}


///////////////////////////////////////////////////////////////
// PRIVATE METHODS
///////////////////////////////////////////////////////////////

void
DatabaseConfig::readConfiguration()
{
    KConfigGroup config = Amarok::config( "MySQL" );

    kcfg_UseInternalDB->setChecked( !config.readEntry( "UseServer", false ) );

    kcfg_Server->setText( config.readEntry( "Host", "localhost" ).toUtf8() );
    kcfg_Port->setValue( config.readEntry( "Port", "3306" ).toInt() );
    kcfg_DBName->setText( config.readEntry( "Database", "amarokdb" ).toUtf8() );

    kcfg_Username->setText( config.readEntry( "User", "amarokuser" ).toUtf8() );
    kcfg_Password->setText( config.readEntry( "Password", "" ).toUtf8() );


    toggleExternalConfigAvailable( kcfg_UseInternalDB->checkState() );
    updateSQLQuery();
}

void
DatabaseConfig::writeConfiguration()
{
    KConfigGroup config = Amarok::config( "MySQL" );

    const bool useExternal = kcfg_UseInternalDB->checkState() != Qt::Checked;

    config.writeEntry( "UseServer", useExternal );

    if( useExternal )
    {
        config.writeEntry( "Host",     kcfg_Server->text() );
        config.writeEntry( "Port",     kcfg_Port->value() );
        config.writeEntry( "Database", kcfg_DBName->text() );
        config.writeEntry( "User",     kcfg_Username->text() );
        config.writeEntry( "Password", kcfg_Password->text() );
    }
}


void
DatabaseConfig::toggleExternalConfigAvailable( const int checkBoxState ) //SLOT
{
    const bool enableExternalConfig = checkBoxState != Qt::Checked;

    label_DatabaseEngine->setEnabled( enableExternalConfig );
    kcfg_DatabaseEngine->setEnabled( enableExternalConfig );
    group_Connection->setVisible( enableExternalConfig );

}

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
        query = QString( "GRANT ALL ON %1.* TO '%2'@'%3' IDENTIFIED BY '%4';\nFLUSH PRIVILEGES;" )
                   .arg( kcfg_DBName->text() )
                   .arg( kcfg_Username->text() )
                   .arg( kcfg_Server->text() )
                   .arg( examplePassword );
    }
    text_SQL->setPlainText( query );
}


inline bool
DatabaseConfig::isSQLInfoPresent() const
{
    return !kcfg_DBName->text().isEmpty() && !kcfg_Username->text().isEmpty() && !kcfg_Server->text().isEmpty();
}


#include "DatabaseConfig.moc"


