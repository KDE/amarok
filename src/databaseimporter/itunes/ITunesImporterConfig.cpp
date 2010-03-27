/****************************************************************************************
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 * Copyright (c) 2008 Leo Franchi <lfranchi@kde.org>                                    *
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

#include "ITunesImporterConfig.h"

#include "core/support/Debug.h"

#include <QCompleter>
#include <QDirModel>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDesktopServices>
#include <KLocale>

ITunesImporterConfig::ITunesImporterConfig( QWidget *parent )
    : DatabaseImporterConfig( parent )
{
    QWidget *gridHolder = new QWidget( this );

    QGridLayout *databaseLayout = new QGridLayout( gridHolder );

    m_databaseLocationLabel = new QLabel( i18n("Database Location"), gridHolder );
    m_databaseLocationInput = new QLineEdit( gridHolder );
    QCompleter *completer = new QCompleter( this );
    completer->setModel( new QDirModel( completer ) );
    m_databaseLocationInput->setCompleter( completer );

    m_databaseLocationInput->setText( QDesktopServices::storageLocation( QDesktopServices::MusicLocation ) + QDir::toNativeSeparators("/iTunes/iTunes Music Library.xml") );

    databaseLayout->addWidget( m_databaseLocationLabel, 5, 0 );
    databaseLayout->addWidget( m_databaseLocationInput, 5, 1 );

    gridHolder->setLayout( databaseLayout );
    
    QWidget *spacer = new QWidget( this );
    spacer->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );
}

#include "ITunesImporterConfig.moc"

