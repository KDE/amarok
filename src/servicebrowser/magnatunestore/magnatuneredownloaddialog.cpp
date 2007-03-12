/*
 Copyright (c) 2006  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*/

#include "debug.h"
#include "magnatuneredownloaddialog.h"

#include <QPushButton>
#include <k3listview.h>

MagnatuneRedownloadDialog::MagnatuneRedownloadDialog(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
: magnatuneReDownloadDialogBase(parent,name, modal,fl)
{
    redownloadButton->setEnabled ( false );

    redownloadListView->setColumnWidthMode( 0, Q3ListView::Manual );
    redownloadListView->setResizeMode( Q3ListView::LastColumn );
}

MagnatuneRedownloadDialog::~MagnatuneRedownloadDialog()
{
}

void MagnatuneRedownloadDialog::setRedownloadItems( const QStringList &items )
{

     QStringListIterator it(items);
     while ( it.hasNext() ) {

           QString currentItem = it.next();
           debug() << "Adding item to redownload dialog: " << currentItem << endl;
           new Q3ListViewItem(redownloadListView, currentItem);
     }

     debug() << "Nothing more to add..." << endl;

}

void MagnatuneRedownloadDialog::redownload( )
{
    emit ( redownload( redownloadListView->currentItem()->text( 0 ) ) );

    hide();
}

void MagnatuneRedownloadDialog::reject( )
{
    hide();
    emit(cancelled());
}

void MagnatuneRedownloadDialog::selectionChanged( )
{
    if (redownloadListView->currentItem() != 0) {
        redownloadButton->setEnabled ( true );
    } else { 
        redownloadButton->setEnabled ( false );
    }
}

/*$SPECIALIZATION$*/


#include "magnatuneredownloaddialog.moc"

