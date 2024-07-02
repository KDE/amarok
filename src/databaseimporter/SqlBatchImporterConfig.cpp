/****************************************************************************************
 * Copyright (c) 2010 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#include "SqlBatchImporterConfig.h"

#include "core/support/Amarok.h"

#include <KLocalizedString>

#include <QComboBox>
#include <QCompleter>
#include <QDirModel>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QFileSystemModel>

SqlBatchImporterConfig::SqlBatchImporterConfig( QWidget *parent )
    : BoxWidget( true, parent )
{
    QWidget *gridHolder = new QWidget( this );

    QGridLayout *databaseLayout = new QGridLayout( gridHolder );

    QLabel *explanationLabel = new QLabel( i18n( "Input file produced by amarokcollectionscanner.<br>"
                                                 "See <a href=\"http://community.kde.org/Amarok/Development/BatchMode\">Batch Mode</a>." ), gridHolder );
    explanationLabel->setTextFormat( Qt::RichText );
    explanationLabel->setAlignment( Qt::AlignHCenter );
    explanationLabel->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::Minimum ); // Don't stretch vertically
    explanationLabel->setMargin( 10 );

    QLabel *label = new QLabel( i18n( "Input file" ), gridHolder );
    m_inputFilePathInput = new QLineEdit( gridHolder );
    QCompleter *completer = new QCompleter( this );
    completer->setModel( new QFileSystemModel( completer ) );
    m_inputFilePathInput->setCompleter( completer );
    m_inputFilePathInput->setText( QDir::homePath() + QStringLiteral("/result.xml") );
    label->setBuddy( m_inputFilePathInput );

    databaseLayout->addWidget( explanationLabel, 0, 0, 1, 2 );
    databaseLayout->addWidget( label, 1, 0 );
    databaseLayout->addWidget( m_inputFilePathInput, 1, 1 );

    gridHolder->setLayout( databaseLayout );

    QWidget *spacer = new QWidget( this );
    spacer->setSizePolicy( QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding );
}

QString
SqlBatchImporterConfig::inputFilePath() const
{
    return m_inputFilePathInput->text();
}
