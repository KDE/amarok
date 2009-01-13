/*******************************************************************************
* copyright              : (C) 2008 Seb Ruiz <ruiz@kde.org>                    *
* copyright              : (C) 2008 Leo Franchi <lfranchi@kde.org>             *
********************************************************************************/

/*******************************************************************************
*                                                                              *
*   This program is free software; you can redistribute it and/or modify       *
*   it under the terms of the GNU General Public License as published by       *
*   the Free Software Foundation; either version 2 of the License, or          *
*   (at your option) any later version.                                        *
*                                                                              *
********************************************************************************/


#include "ITunesImporterConfig.h"

#include "Debug.h"

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

