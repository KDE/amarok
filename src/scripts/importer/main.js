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

dlg        = new QDialog();
mainLayout = new QVBoxLayout();

databaseLayout = new QGridLayout();

sqlTypeLabel = new QLabel( dlg );
sqlTypeLabel.setText( "Database" );
sqlTypeCombo = new QComboBox( dlg );
sqlTypeCombo.addItem( "SQLite" );
sqlTypeCombo.addItem( "MySQL" );
sqlTypeCombo.addItem( "PostgreSQL" );

locationLabel = new QLabel( dlg );
locationLabel.setText( "Location" );
locationEdit = new QLineEdit( dlg );
locationEdit.setText( "~/.kde/share/apps/amarok/collection.db" );

usernameLabel = new QLabel( dlg );
usernameLabel.setText( "Username" );
usernameEdit = new QLineEdit( dlg );

passwordLabel = new QLabel( dlg );
passwordLabel.setText( "Password" );
passwordEdit = new QLineEdit( dlg );
//passwordEdit.setEchoMode( QLineEdit.Password );

databaseLayout.addWidget( sqlTypeLabel, 0, 0 );
databaseLayout.addWidget( sqlTypeCombo, 0, 1 );
databaseLayout.addWidget( locationLabel, 1, 0 );
databaseLayout.addWidget( locationEdit, 1, 1 );
databaseLayout.addWidget( usernameLabel, 2, 0 );
databaseLayout.addWidget( usernameEdit, 2, 1 );
databaseLayout.addWidget( passwordLabel, 3, 0 );
databaseLayout.addWidget( passwordEdit, 3, 1 );

sqlTypeCombo.currentIndexChanged.connect( databaseTypeChanged );
databaseTypeChanged( sqlTypeCombo.currentText ); // make sure the correct input fields are visible

importArtwork = new QCheckBox( "Import downloaded artwork", dlg );

buttonBox = new QDialogButtonBox();
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

dlg.show();

if( dlg.exec() == QDialog.REJECTED )
{
    Debug.debug( "Cancelled" );
    return;
}

Debug.debug( "Will proceed to convert stats" );

var db = QSqlDatabase.addDatabase( "QSQLITE", "a1" );
db.setDatabaseName( collectionEdit.text() );
db.open();

Debug.debug( "Fetching devices from Amarok 1.4" );
query = db.exec( "SELECT id, lastmountpoint FROM devices" );

/**
 * HELPER FUNCTIONS
 **/

function transferData( query )
{
    while( query.next() )
    {
        id = query.value(0).toString();
        lmp = query.value(1).toString();
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
}