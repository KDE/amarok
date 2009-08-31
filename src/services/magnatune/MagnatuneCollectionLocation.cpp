/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                    *
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
 
#include "MagnatuneCollectionLocation.h"

#include <KDialog>
#include <KLocale>

#include <QLabel>

MagnatuneCollectionLocation::MagnatuneCollectionLocation( MagnatuneSqlCollection const *parentCollection )
    : ServiceCollectionLocation( parentCollection )
{
}


MagnatuneCollectionLocation::~MagnatuneCollectionLocation()
{
}

void MagnatuneCollectionLocation::showSourceDialog( const Meta::TrackList & tracks, bool removeSources )
{
    Q_UNUSED( tracks );
    Q_UNUSED( removeSources );

    KDialog dialog;
    dialog.setCaption( i18n( "Preview Tracks" ) );
    dialog.setButtons( KDialog::Ok | KDialog::Cancel );

    QLabel *label = new QLabel( i18n( "The tracks you are about to copy are Magnatune.com preview streams. For better quality and advert free streams, consider buying an album download. Remember that when buying from Magnatune the artist gets 50%. Also if you buy using Amarok, you support the Amarok project with 10%." ) );

    label->setWordWrap ( true );
    label->setMaximumWidth( 400 );
    
    dialog.setMainWidget( label );

    dialog.exec();

    if ( dialog.result() == QDialog::Rejected )
        abort();

    slotShowSourceDialogDone();
    return;
}
