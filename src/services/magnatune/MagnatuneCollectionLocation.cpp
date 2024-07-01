/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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
 
#include "MagnatuneCollectionLocation.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QVBoxLayout>

#include <KLocalizedString>

using namespace Collections;

MagnatuneCollectionLocation::MagnatuneCollectionLocation( MagnatuneSqlCollection *parentCollection )
    : ServiceCollectionLocation( parentCollection )
{
}


MagnatuneCollectionLocation::~MagnatuneCollectionLocation()
{
}

void MagnatuneCollectionLocation::showSourceDialog( const Meta::TrackList &tracks, bool removeSources )
{
    QDialog dialog;
    dialog.setWindowTitle( i18n( "Preview Tracks" ) );

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    QVBoxLayout *mainLayout = new QVBoxLayout;
    dialog.setLayout(mainLayout);

    QPushButton *okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setDefault(true);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    QLabel *label = new QLabel( i18n( "The tracks you are about to copy are Magnatune.com preview streams. For better quality and advert free streams, consider buying an album download. Remember that when buying from Magnatune the artist gets 50%. Also if you buy using Amarok, you support the Amarok project with 10%." ) );

    label->setWordWrap ( true );
    label->setMaximumWidth( 400 );
    
    mainLayout->addWidget(label);
    mainLayout->addWidget(buttonBox);
    dialog.exec();

    if ( dialog.result() == QDialog::Rejected )
        abort();

    CollectionLocation::showSourceDialog( tracks, removeSources ); // to get transcoding dialog
}
