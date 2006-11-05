//
// C++ Implementation: 
//
// Description: 
//
//
// Author: Mark Kretschmann <markey@web.de>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution
//
//


#include "magnatuneredownloaddialog.h"

#include <qpushbutton.h>
#include <qlistview.h>

MagnatuneRedownloadDialog::MagnatuneRedownloadDialog(QWidget* parent, const char* name, bool modal, WFlags fl)
: magnatuneReDownloadDialogBase(parent,name, modal,fl)
{
    redownloadButton->setEnabled ( false );
}

MagnatuneRedownloadDialog::~MagnatuneRedownloadDialog()
{
}

void MagnatuneRedownloadDialog::setRedownloadItems( QStringList items )
{

    for ( QStringList::Iterator it = items.begin(); it != items.end(); ++it ) {
       new QListViewItem(redownloadListView, (*it));
    }

}

void MagnatuneRedownloadDialog::redownload( )
{
    emit ( redownload( redownloadListView->currentItem()->text( 0 ) ) );
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

