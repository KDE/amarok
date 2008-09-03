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
Importer.loadQtBinding( "qt.gui" );
// Importer.include( "qtscript_debug/debug" );

// Debug.initialize();
// Debug.app_name = "Importer";

print( "Starting importer" );

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
locationEdit.setText( QDir.homePath() + "/.kde/share/apps/amarok/collection.db" );

var usernameLabel = new QLabel( dlg );
usernameLabel.setText( "Username" );
var usernameEdit = new QLineEdit( dlg );

var passwordLabel = new QLabel( dlg );
passwordLabel.setText( "Password" );
var passwordEdit = new QLineEdit( dlg );
passwordEdit.echoMode = QLineEdit.Password;

var dbNameLabel = new QLabel( dlg );
dbNameLabel.setText( "Database" );
var dbNameEdit = new QLineEdit( dlg );

var hostnameLabel = new QLabel( dlg );
hostnameLabel.setText( "Hostname" );
var hostnameEdit = new QLineEdit( dlg );
hostnameEdit.setText( "localhost" );

usernameEdit.text = "amarok";
passwordEdit.text = "rockin_db";
dbNameEdit.text   = "amarokdb";

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
    print( "Cancelled" );
    return;
}

print( "Will proceed to convert stats" );

var db; // this will become the QSql database connection

print( "Selected database connection:", sqlTypeCombo.currentText );

if( sqlTypeCombo.currentText == "SQLite" )
{
    print( "Location:", locationEdit.text );
    db = QSqlDatabase.addDatabase( "QSQLITE", "amarok1" /* just some identifier requried by QSql */ );
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

if( !db.open() )
{
    print( "[ERROR] could not open Amarok 1.4 database" );
    print( "[ERROR]", db.lastError.text );
    return;
}

var a2db = QSqlDatabase.addDatabase( "QSQLITE", "amarok2" /* just some identifier requried by QSql */ );
a2db.setDatabaseName( QDir.homePath() + "/.kde4/share/apps/amarok/collection.db" );

if( !a2db.open() )
{
    print( "[ERROR] could not open Amarok 2 database" );
    print( "[ERROR]", db.lastError.text );
    return;
}

transfer();

/**
 * HELPER FUNCTIONS
 **/

function transfer()
{
    print( "Fetching tracks from Amarok 1.4" );
    var query = db.exec( "SELECT lastmountpoint, url, createdate, accessdate, percentage, rating, playcounter " +
                         "FROM statistics S, devices D where S.deviceid = D.id" );

    var a2_url = a2db.exec();
    a2_url.prepare( "SELECT id FROM urls WHERE rpath = :rpath" );

    var a1_insert = db.exec();
    a1_insert.prepare( "INSERT INTO statistics( url, createdate, accessdate, score, rating, playcount ) " +
                       "VALUES ( :urlid, :createdate, :accessdate, :score, :rating, :playcount )" );

    //var staleValues = new QVariantList();
    
    while( query.next() )
    {
        var result;
        
        var index = 0;
        var mount = query.value(index++).toString();
        var url   = query.value(index++);
        var createdate = query.value(index++).toString();
        var accessdate = query.value(index++).toString();
        var score = query.value(index++);
        var rating = query.value(index++);
        var playcount = query.value(index++);
        
        // make the url absolute path
        print( "url: " + url );
        print( "toString: " + url.toString() );
        print( "data: " + url.data() );
        url = mount + url.substring(1);

        // then make it "relative" again, for Amarok 2 devices or something
        url = "." + url;

        a2_url.bindValue( ":rpath", url, QSql.In );
        result = a2_url.exec();
        if( !result )
        {
            print( "Insertion failed", a2_url.executedQuery() );
            continue;
        }
            

        if( !a2_url.next() ) // couldn't the url in the database
        {
            print( "Stale entry:", url );
            //staleValues.append( url );
            continue;
        }

        a1_insert.bindValue( ":urlid", url, QSql.In );
        a1_insert.bindValue( ":createdate", createdate, QSql.In );
        a1_insert.bindValue( ":accessdate", accessdate, QSql.In );
        a1_insert.bindValue( ":score", score, QSql.In );
        a1_insert.bindValue( ":rating", rating, QSql.In );
        a1_insert.bindValue( ":playcount", playcount, QSql.In );
        result = a1_insert.exec();

        if( !result )
        {
            print( "Insertion failed", a1_insert.executedQuery() );
            continue;
        }
        print( "Updated", url );
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