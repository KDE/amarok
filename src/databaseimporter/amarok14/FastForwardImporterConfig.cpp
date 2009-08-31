/****************************************************************************************
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "FastForwardImporterConfig.h"

#include "Amarok.h"
#include "Debug.h"

#include <QComboBox>
#include <QCompleter>
#include <QDirModel>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>

FastForwardImporterConfig::FastForwardImporterConfig( QWidget *parent )
    : DatabaseImporterConfig( parent )
{
    QWidget *gridHolder = new QWidget( this );

    QGridLayout *databaseLayout = new QGridLayout( gridHolder );

    QLabel *connectionLabel = new QLabel( i18n("Connection"), gridHolder );
    m_connectionCombo = new QComboBox( gridHolder );
    m_connectionCombo->insertItem( 0, "SQLite", FastForwardImporter::SQLite );
    m_connectionCombo->insertItem( 1, "MySQL", FastForwardImporter::MySQL );
    m_connectionCombo->insertItem( 2, "PostgreSQL", FastForwardImporter::PostgreSQL );

    m_databaseLocationLabel = new QLabel( i18n("Database Location"), gridHolder );
    m_databaseLocationInput = new QLineEdit( gridHolder );
    QCompleter *completer = new QCompleter( this );
    completer->setModel( new QDirModel( completer ) );
    m_databaseLocationInput->setCompleter( completer );
    m_databaseLocationInput->setText( QDir::homePath() + "/.kde/share/apps/amarok/collection.db" );
    m_databaseLocationLabel->setBuddy( m_databaseLocationInput );

    m_usernameLabel = new QLabel( i18n("Username"), gridHolder );
    m_usernameInput = new QLineEdit( gridHolder );
    m_usernameLabel->setBuddy( m_usernameInput );

    m_passwordLabel = new QLabel( i18n("Password"), gridHolder );
    m_passwordInput = new QLineEdit( gridHolder );
    m_passwordInput->setEchoMode( QLineEdit::Password );
    m_passwordLabel->setBuddy( m_passwordInput );

    m_databaseLabel = new QLabel( i18n("Database Name"), gridHolder );
    m_databaseInput = new QLineEdit( gridHolder );
    m_databaseLabel->setBuddy( m_databaseInput );

    m_hostnameLabel = new QLabel( i18n("Hostname"), gridHolder );
    m_hostnameInput = new QLineEdit( gridHolder );
    m_hostnameInput->setText( "localhost" );
    m_hostnameLabel->setBuddy( m_hostnameInput );

    databaseLayout->addWidget( connectionLabel, 0, 0 );
    databaseLayout->addWidget( m_connectionCombo, 0, 1 );

    databaseLayout->addWidget( m_usernameLabel, 1, 0 );
    databaseLayout->addWidget( m_usernameInput, 1, 1 );
    databaseLayout->addWidget( m_passwordLabel, 2, 0 );
    databaseLayout->addWidget( m_passwordInput, 2, 1 );
    
    databaseLayout->addWidget( m_databaseLabel, 3, 0 );
    databaseLayout->addWidget( m_databaseInput, 3, 1 );
    databaseLayout->addWidget( m_hostnameLabel, 4, 0 );
    databaseLayout->addWidget( m_hostnameInput, 4, 1 );

    databaseLayout->addWidget( m_databaseLocationLabel, 5, 0 );
    databaseLayout->addWidget( m_databaseLocationInput, 5, 1 );

    gridHolder->setLayout( databaseLayout );

    connect( m_connectionCombo, SIGNAL( currentIndexChanged(int) ), SLOT( connectionChanged(int) ) );
    connectionChanged( m_connectionCombo->currentIndex() ); // Make sure we sync the UI as appropriate

    m_smartMatchCheck = new QCheckBox( i18n("Match tracks by meta tags"), this );
    m_smartMatchCheck->setChecked( true );

    m_importArtworkCheck = new QCheckBox( i18n("Import downloaded artwork"), this );
    m_importArtworkCheck->setChecked( true );
    
    const QString oldCoverPath = Amarok::saveLocation( "albumcovers/large/" ).replace("kde4", "kde");

    QWidget *artworkDirHolder = new QWidget( this );

    QGridLayout *artworkDirLayout = new QGridLayout( artworkDirHolder );

    QLabel *artworkDirLabel = new QLabel( i18n("Artwork directory"), artworkDirHolder );
    m_importArtworkDirInput = new QLineEdit( artworkDirHolder );
    m_importArtworkDirInput->setText( oldCoverPath );

    artworkDirLayout->addWidget( artworkDirLabel, 0, 0 );
    artworkDirLayout->addWidget( m_importArtworkDirInput, 0, 1 );

    artworkDirHolder->setLayout( artworkDirLayout );

    connect( m_importArtworkCheck, SIGNAL( stateChanged(int) ), SLOT( importArtworkChanged(int) ) );

    QWidget *spacer = new QWidget( this );
    spacer->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );
}

FastForwardImporter::ConnectionType
FastForwardImporterConfig::connectionType() const
{
    const int index = m_connectionCombo->currentIndex();
    bool isOK = true;
    int connType = FastForwardImporter::SQLite;
    if ( index == -1 )
        return FastForwardImporter::ConnectionType( connType );

    QVariant itemData = m_connectionCombo->itemData( index );
    if ( itemData == QVariant::Invalid )
        return FastForwardImporter::ConnectionType( connType );

    connType = itemData.toInt( &isOK );
    if ( !isOK ||
         ( connType < FastForwardImporter::SQLite ||
           connType > FastForwardImporter::PostgreSQL ) )
    {
        connType = FastForwardImporter::SQLite;
    }

    return FastForwardImporter::ConnectionType( connType );
}

void
FastForwardImporterConfig::connectionChanged( int index )
{
    const int type = m_connectionCombo->itemData( index ).toInt();

    switch( type )
    {
        case FastForwardImporter::SQLite:
            m_databaseLabel->hide();
            m_hostnameLabel->hide();
            m_usernameLabel->hide();
            m_passwordLabel->hide();
            m_databaseInput->hide();
            m_hostnameInput->hide();
            m_usernameInput->hide();
            m_passwordInput->hide();
            m_databaseLocationLabel->show();
            m_databaseLocationInput->show();
            break;

        case FastForwardImporter::MySQL:
        case FastForwardImporter::PostgreSQL:
            m_databaseLabel->show();
            m_hostnameLabel->show();
            m_usernameLabel->show();
            m_passwordLabel->show();
            m_databaseInput->show();
            m_hostnameInput->show();
            m_usernameInput->show();
            m_passwordInput->show();
            m_databaseLocationLabel->hide();
            m_databaseLocationInput->hide();
            break;
    }
}

void
FastForwardImporterConfig::importArtworkChanged( int state )
{
    m_importArtworkDirInput->setEnabled( state == Qt::Checked );
}

#include "FastForwardImporterConfig.moc"

