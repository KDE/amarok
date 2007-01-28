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


#include "magnatuneredownloaddialog.h"

#include <qpushbutton.h>
#include <klistview.h>

MagnatuneRedownloadDialog::MagnatuneRedownloadDialog(QWidget* parent, const char* name, bool modal, WFlags fl)
: magnatuneReDownloadDialogBase(parent,name, modal,fl)
{
    redownloadButton->setEnabled ( false );

    redownloadListView->setColumnWidthMode( 0, QListView::Manual );
    redownloadListView->setResizeMode( QListView::LastColumn );
}

MagnatuneRedownloadDialog::~MagnatuneRedownloadDialog()
{
}

void MagnatuneRedownloadDialog::setRedownloadItems( QStringList items )
{

    for ( QStringList::Iterator it = items.begin(); it != items.end(); ++it ) {
       new KListViewItem(redownloadListView, (*it));
    }

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

