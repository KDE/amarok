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

#include <QGridLayout>
#include <QPushButton>

FilenameLayoutDialog::FilenameLayoutDialog( QWidget *parent )
    : KDialog( parent )
{
    setupUi( this );

    setCaption( i18n( "Filename Layout Chooser" ) );
    setButtons( KDialog::Ok | KDialog::Cancel );
    setMainWidget( widget );

    caseEditRadioButtons << rbAllUpper << rbAllLower << rbFirstLetter << rbTitleCase;

    connect( cbCase, SIGNAL( toggled( bool ) ),
             this, SLOT( editStateEnable( bool ) ) );
}

QString
FilenameLayoutDialog::getParsableScheme()
{
    return filenameLayout->getParsableScheme();
}

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
