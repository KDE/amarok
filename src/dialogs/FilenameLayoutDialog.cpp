/******************************************************************************
 * Copyright (C) 2008 Teo Mrnjavac <teo.mrnjavac@gmail.com>                   *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License, or (at your option) any later version.                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
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

FilenameLayoutDialog::FilenameLayoutDialog( QWidget *parent ) : KDialog( parent )
{
    setupUi( this );

    setCaption( "Filename Layout Chooser" );
    setButtons( 0 );

    tokenPool->addItem( tr( "Track #" ) );
    tokenPool->addItem( tr( "Track Name" ) );
    tokenPool->addItem( tr( "Disc #" ) );
    tokenPool->addItem( tr( "Track Count" ) );
    tokenPool->addItem( tr( "Disc Count" ) );
    tokenPool->addItem( tr( "Artist" ) );
    tokenPool->addItem( tr( "Composer" ) );
    tokenPool->addItem( tr( "Year" ) );
    tokenPool->addItem( tr( "Album Name" ) );
    tokenPool->addItem( tr( "Comment" ) );

    QGridLayout *dialogLayout = new QGridLayout;
    this->setLayout( dialogLayout );

    QPushButton *debugButton = new QPushButton( "TEST" );
    connect( debugButton, SIGNAL( clicked() ),
            frame, SLOT( slotAddToken() ) );

    dialogLayout->addWidget( debugButton );
    debugButton->show();
}
