/**************************************************************************
*   Amarok 2 script to import Amarok 1.4 data                             *
*                                                                         *
*   Copyright                                                             *
*   (C) 2008 Seb Ruiz <ruiz@kde.org>                                      *
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
**************************************************************************/

// "Importer" is ambiguous here. This means the QtSql importer.
Importer.loadQtBinding( "qt.core" );
Importer.loadQtBinding( "qt.sql" );
Importer.load( "qtscript_debug/debug" );

Debug.initialize();
Debug.app_name = "Importer";

Debug.debug( "Starting importer" );

var dlg        = new QDialog();
var mainLayout = new QVBoxLayout();

var databaseLayout = new QGridLayout();

var sqlTypeLabel = new QLabel( dlg );
sqlTypeLabel.setText( "Connection" );
var sqlTypeCombo = new QComboBox( dlg );
sqlTypeCombo.addItem( "SQLite" );
sqlTypeCombo.addItem( "MySQL" );
sqlTypeCombo.addItem( "PostgreSQL" );

var locationLabel = new QLabel( dlg );
locationLabel.setText( "Location" );
var locationEdit = new QLineEdit( dlg );
locationEdit.setText( "~/.kde/share/apps/amarok/collection.db" );

var usernameLabel = new QLabel( dlg );
usernameLabel.setText( "Username" );
var usernameEdit = new QLineEdit( dlg );

var passwordLabel = new QLabel( dlg );
passwordLabel.setText( "Password" );
var passwordEdit = new QLineEdit( dlg );

var dbNameLabel = new QLabel( dlg );
dbNameLabel.setText( "Database" );
var dbNameEdit = new QLineEdit( dlg );

var hostnameLabel = new QLabel( dlg );
hostnameLabel.setText( "Hostname" );
var hostnameEdit = new QLineEdit( dlg );
hostnameEdit.setText( "localhost" );

//passwordEdit.setEchoMode( QLineEdit.Password );

databaseLayout.addWidget( sqlTypeLabel, 0, 0 );
databaseLayout.addWidget( sqlTypeCombo, 0, 1 );

databaseLayout.addWidget( locationLabel, 1, 0 );
databaseLayout.addWidget( locationEdit, 1, 1 );

databaseLayout.addWidget( usernameLabel, 2, 0 );
databaseLayout.addWidget( usernameEdit, 2, 1 );
databaseLayout.addWidget( passwordLabel, 3, 0 );
databaseLayout.addWidget( passwordEdit, 3, 1 );

databaseLayout.addWidget( dbNameLabel, 4, 0 );
databaseLayout.addWidget( dbNameEdit, 4, 1 );
databaseLayout.addWidget( hostnameLabel, 5, 0 );
databaseLayout.addWidget( hostnameEdit, 5, 1 );

sqlTypeCombo.currentIndexChanged.connect( databaseTypeChanged );
databaseTypeChanged( sqlTypeCombo.currentText ); // make sure the correct input fields are visible

var importArtwork = new QCheckBox( "Import downloaded artwork", dlg );

var buttonBox = new QDialogButtonBox();
buttonBox.addButton( QDialogButtonBox.Ok );
buttonBox.addButton( QDialogButtonBox.Cancel );

// connect the buttons to the dialog actions
buttonBox.accepted.connect( dlg.accept );
buttonBox.rejected.connect( dlg.reject );

mainLayout.addLayout( databaseLayout );
mainLayout.addWidget( importArtwork, 0, 0 );
mainLayout.addStretch();
mainLayout.addWidget( buttonBox, 0, 0 );

dlg.setLayout( mainLayout );
dlg.setMinimumSize( 400, 300 );

dlg.show();


if( dlg.exec() == QDialog.REJECTED )
{
    Debug.debug( "Cancelled" );
    return;
}

Debug.debug( "Will proceed to convert stats" );

var db; // this will become the QSql database connection

print( "Selected database connection:", sqlTypeCombo.currentText );
print( "Location:", locationEdit.text );

if( sqlTypeCombo.currentText == "SQLite" )
{
    db = QSqlDatabase.addDatabase( "QSQLITE", "sqlite" /* just some identifier requried by QSql */ );
    db.setDatabaseName( locationEdit.text );
}
else if( sqlTypeCombo.currentText == "MySQL" )
{
    db = QSqlDatabase.addDatabase( "QMYSQL", "mysql" );
    db.setDatabaseName( dbNameEdit.text );
    db.setHostName( hostnameEdit.text );
    db.setUserName( usernameEdit.text );
    db.setPassword( passwordEdit.text );
}
else if( sqlTypeCombo.currentText == "PostgreSQL" )
{
    db = QSqlDatabase.addDatabase( "QPSQL", "psql" );
    db.setDatabaseName( dbNameEdit.text );
    db.setHostName( hostnameEdit.text );
    db.setUserName( usernameEdit.text );
    db.setPassword( passwordEdit.text );
}

db.open();

Debug.debug( "Fetching devices from Amarok 1.4" );
var query = db.exec( "SELECT id, lastmountpoint FROM devices" );

transferData( query );

/**
 * HELPER FUNCTIONS
 **/

function transferData( query )
{
    while( query.next() )
    {
        var id = query.value(0).toString();
        var lmp = query.value(1).toString();
        print( id + " : " + lmp );
    }
}

function databaseTypeChanged( dbType )
{
    locationLabel.setVisible( dbType == "SQLite" );
    locationEdit.setVisible( dbType == "SQLite" );

    usernameLabel.setVisible( dbType != "SQLite" );
    usernameEdit.setVisible( dbType != "SQLite" );

    passwordLabel.setVisible( dbType != "SQLite" );
    passwordEdit.setVisible( dbType != "SQLite" );

    dbNameLabel.setVisible( dbType != "SQLite" );
    dbNameEdit.setVisible( dbType != "SQLite" );

    hostnameLabel.setVisible( dbType != "SQLite" );
    hostnameEdit.setVisible( dbType != "SQLite" );
}