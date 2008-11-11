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

#include <QComboBox>
#include <QCompleter>
#include <QDirModel>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSqlDatabase>

ITunesImporterConfig::ITunesImporterConfig( QWidget *parent )
    : DatabaseImporterConfig( parent )
{
    QWidget *gridHolder = new QWidget( this );

    QGridLayout *databaseLayout = new QGridLayout( gridHolder );

    m_databaseLocationLabel = new QLabel( "Database Location", gridHolder );
    m_databaseLocationInput = new QLineEdit( gridHolder );
    QCompleter *completer = new QCompleter( this );
    completer->setModel( new QDirModel( completer ) );
    m_databaseLocationInput->setCompleter( completer );
#ifdef Q_WS_MAC
    m_databaseLocationInput->setText( QDir::homePath() + "/Music/iTunes/iTunes Music Library.xml" );
#elif Q_WS_WIN
    m_databaseLocationInput->setText( QString::toNativeSeparators( QDir::homePath() + "/My Documents/My Music/iTunes/iTunes Music Library.xml" ) );
#endif
    databaseLayout->addWidget( m_databaseLocationLabel, 5, 0 );
    databaseLayout->addWidget( m_databaseLocationInput, 5, 1 );

    gridHolder->setLayout( databaseLayout );
    
    QWidget *spacer = new QWidget( this );
    spacer->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );
}

#include "ITunesImporterConfig.moc"

