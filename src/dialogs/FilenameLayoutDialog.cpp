/******************************************************************************
 * Copyright (C) 2008 Teo Mrnjavac <teo.mrnjavac@gmail.com>                   *
 *                                                                            *
 * This program is free software; you can redisi18nibute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is disi18nibuted in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/
#include "FilenameLayoutDialog.h"

#include "Amarok.h"
#include "Debug.h"

#include <KConfig>
#include <KConfigGroup>
#include <kstandarddirs.h>

#include <QGridLayout>
#include <QPushButton>

FilenameLayoutDialog::FilenameLayoutDialog( QWidget *parent )
    : QWidget( parent )
{
    setupUi( this );
    optionsFrame->hide();

    caseEditRadioButtons << rbAllUpper << rbAllLower << rbFirstLetter << rbTitleCase;

    filenameLayoutEdit->hide();
    syntaxLabel->hide();
    QString * hintImagePath = new QString( KStandardDirs::locate( "data", "amarok/images/FilenameLayoutDialogHint.png" ) );
    QPixmap * hintImage = new QPixmap( *hintImagePath );
    hintPicture->setPixmap( *hintImage );

    connect( cbCase, SIGNAL( toggled( bool ) ),
             this, SLOT( editStateEnable( bool ) ) );
    connect( parent, SIGNAL( accepted() ),
             this, SLOT( onAccept() ) );
    connect( tokenPool, SIGNAL( onDoubleClick( QString ) ),
             filenameLayout, SLOT( addToken( QString ) ) );
    connect( kpbAdvanced, SIGNAL( clicked() ),
             this, SLOT( toAdvancedMode() ) );

    //KConfig stuff:
    int caseOptions = Amarok::config( "TagGuesser" ).readEntry( "Case options" ).toInt();
    if( !caseOptions )
        cbCase->setChecked( false );
    else
    {
        cbCase->setChecked( true );
        if( caseOptions == 1 )
            rbAllLower->setChecked( true );
        else if( caseOptions == 2 )
            rbAllUpper->setChecked( true );
        else if( caseOptions == 3 )
            rbFirstLetter->setChecked( true );
        else if( caseOptions == 4 )
            rbTitleCase->setChecked( true );
        else
            debug() << "OUCH";
    }
    int whitespaceOptions = Amarok::config( "TagGuesser" ).readEntry( "Eliminate trailing spaces" ).toInt();
    cbEliminateSpaces->setChecked( whitespaceOptions );
    int underscoreOptions = Amarok::config( "TagGuesser" ).readEntry( "Replace underscores" ).toInt();
    cbReplaceUnderscores->setChecked( underscoreOptions );
}

//Stores the configuration when the dialog is accepted.
void
FilenameLayoutDialog::onAccept()    //SLOT
{
    Amarok::config( "TagGuesser" ).writeEntry( "Case options", getCaseOptions() );
    Amarok::config( "TagGuesser" ).writeEntry( "Eliminate trailing spaces", getWhitespaceOptions() );
    Amarok::config( "TagGuesser" ).writeEntry( "Replace underscores", getUnderscoreOptions() );
}

//Forwards the request for a scheme to FilenameLayoutWidget
QString
FilenameLayoutDialog::getParsableScheme()
{
    if( kpbAdvanced->text() == i18n( "&Basic..." ) )
    {
        return filenameLayoutEdit->text();
    }
    return filenameLayout->getParsableScheme();
}

//Handles the radiobuttons
void
FilenameLayoutDialog::editStateEnable( bool checked )      //SLOT
{
    if( !checked )
    {
        foreach( QRadioButton *rb, caseEditRadioButtons )
        {
            rb->setEnabled( false );
        }
    }
    else
    {
        foreach( QRadioButton *rb, caseEditRadioButtons )
        {
            rb->setEnabled( true );
        }
    }
}

//Returns a code for the configuration.
int
FilenameLayoutDialog::getCaseOptions()
{
    //Amarok::config( "TagGuesser" ).readEntry( "Filename schemes", QStringList() );
    if( !cbCase->isChecked() )
        return 0;
    else
    {
        if( rbAllLower->isChecked() )
            return 1;
        else if( rbAllUpper->isChecked() )
            return 2;
        else if( rbFirstLetter->isChecked() )
            return 3;
        else if( rbTitleCase->isChecked() )
            return 4;
        else
        {
            debug() << "OUCH!";
            return 99;
        }
    }
}

//As above
int
FilenameLayoutDialog::getWhitespaceOptions()
{
    if( !cbEliminateSpaces->isChecked() )
        return 0;
    else
        return 1;
}

//As above
int
FilenameLayoutDialog::getUnderscoreOptions()
{
    if( !cbReplaceUnderscores->isChecked() )
        return 0;
    else
        return 1;
}

//Handles the modifications to the dialog to toggle between advanced and basic editing mode.
void
FilenameLayoutDialog::toAdvancedMode()
{
    if( kpbAdvanced->text() == i18n( "&Advanced..." ) )     //is this a good idea?
    {
        kpbAdvanced->setText( i18n( "&Basic..." ) );
        filenameLayout->hide();
        filenameLayoutEdit->show();
        filenameLayoutEdit->setText( filenameLayout->getParsableScheme() );
        tokenPool->hide();
        hintPicture->hide();
        syntaxLabel->show();
    }
    else
    {
        kpbAdvanced->setText( i18n( "&Advanced..." ) );
        filenameLayoutEdit->hide();
        syntaxLabel->hide();
        filenameLayout->show();
        tokenPool->show();
        hintPicture->show();
    }
}

//The following two show and hide the Options ButtonGroup
void
FilenameLayoutDialog::showOpts()
{
    optionsFrame->show();
}

void
FilenameLayoutDialog::hideOpts()
{
    optionsFrame->hide();
}


