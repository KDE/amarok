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

#include <PluginManager.h>
#include <core/support/Amarok.h>
#include <core/support/Debug.h>
#include "core/support/PluginFactory.h"

#include <KConfigDialogManager>
#include <KMessageBox>
#include <KCMultiDialog>


DatabaseConfig::DatabaseConfig( Amarok2ConfigDialog* parent, KCoreConfigSkeleton *config )
    : ConfigDialogBase( parent )
    , m_configManager( new KConfigDialogManager( this, config ) )
{
    setupUi( this );

    // Fix some weird tab orderness
    setTabOrder( kcfg_Host,     kcfg_Port );        // host to port
    setTabOrder( kcfg_Port,     kcfg_User );        // port to username
    setTabOrder( kcfg_User,     kcfg_Password );    // username to password
    setTabOrder( kcfg_Password, kcfg_Database );    // password to database

    // enable the test button if one of the plugin factories has a correct testSettings slot
    // get all storage factories
    auto factories = Plugins::PluginManager::instance()->factories( Plugins::PluginManager::Storage );
    bool testFunctionAvailable = false;
    for( const auto &factory : factories )
    {
        // check the meta object if there is a testSettings slot available
        if( factory->metaObject()->
            indexOfMethod( QMetaObject::normalizedSignature("testSettings(QString, QString, QString, int, QString)") ) >= 0 )
            testFunctionAvailable = true;
    }
    button_Test->setEnabled( testFunctionAvailable );

    // connect slots
    connect( kcfg_UseServer, &QCheckBox::stateChanged, this, &DatabaseConfig::toggleExternalConfigAvailable );

    connect( kcfg_Database, &QLineEdit::textChanged, this, &DatabaseConfig::updateSQLQuery );
    connect( kcfg_User,     &QLineEdit::textChanged, this, &DatabaseConfig::updateSQLQuery );
    connect( button_Test,   &QAbstractButton::clicked,  this, &DatabaseConfig::testDatabaseConnection );

    toggleExternalConfigAvailable( kcfg_UseServer->checkState() );

    updateSQLQuery();

    m_configManager->addWidget( this );
}

DatabaseConfig::~DatabaseConfig()
{}

void
DatabaseConfig::toggleExternalConfigAvailable( const int checkBoxState ) //SLOT
{
    group_Connection->setEnabled( checkBoxState == Qt::Checked );
}

void
DatabaseConfig::testDatabaseConnection() //SLOT
{
    // get all storage factories
    auto factories = Plugins::PluginManager::instance()->factories( Plugins::PluginManager::Storage );

    // try if they have a testSettings slot that we can call
    for( const auto &factory : factories )
    {
        bool callSucceeded = false;
        QStringList connectionErrors;

        callSucceeded = QMetaObject::invokeMethod( factory.data(),
                               "testSettings",
                               Q_RETURN_ARG( QStringList, connectionErrors ),
                               Q_ARG( QString, kcfg_Host->text() ),
                               Q_ARG( QString, kcfg_User->text() ),
                               Q_ARG( QString, kcfg_Password->text() ),
                               Q_ARG( int, kcfg_Port->text().toInt() ),
                               Q_ARG( QString, kcfg_Database->text() )
                               );

        if( callSucceeded )
        {
            if( connectionErrors.isEmpty() )
                KMessageBox::information(this,
                                         i18n( "Amarok was able to establish a successful connection to the database." ),
                                         i18n( "Success" ) );
            else
                KMessageBox::error( this, i18n( "The amarok database reported "
                                                "the following errors:\n%1\nIn most cases you will need to resolve "
                                                "these errors before Amarok will run properly.",
                                    connectionErrors.join( QStringLiteral("\n") ) ),
                                    i18n( "Database Error" ));
        }
    }
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
{
    if( m_configManager->hasChanged() )
        KMessageBox::information( nullptr,
                 i18n( "Changes to database settings only take\neffect after Amarok is restarted." ),
                 i18n( "Database settings changed" ) );
}


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
        query = QStringLiteral( "CREATE DATABASE %1;\nGRANT ALL PRIVILEGES ON %1.* TO '%2' IDENTIFIED BY '%3'; FLUSH PRIVILEGES;" )
                   .arg( kcfg_Database->text(), kcfg_User->text(), examplePassword );
    }
    text_SQL->setPlainText( query );
}


inline bool
DatabaseConfig::isSQLInfoPresent() const
{
    return !kcfg_Database->text().isEmpty() && !kcfg_User->text().isEmpty() && !kcfg_Host->text().isEmpty();
}




