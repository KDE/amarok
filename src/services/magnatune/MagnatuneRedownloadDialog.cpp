/****************************************************************************************
 * Copyright (c) 2006,2007 Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>               *
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

#include "MagnatuneRedownloadDialog.h"

#include "Debug.h"

#include <QHeaderView>

MagnatuneRedownloadDialog::MagnatuneRedownloadDialog(QWidget* parent, const char* name, bool modal, Qt::WFlags fl)
: QDialog(parent, fl)
{
    setObjectName( name );
    setModal( modal );
    setupUi(this);
    redownloadButton->setEnabled ( false );

    redownloadListView->header()->setStretchLastSection( true );
    redownloadListView->setRootIsDecorated( false );
    connect( redownloadListView, SIGNAL(itemSelectionChanged()), SLOT(selectionChanged()) );
}

MagnatuneRedownloadDialog::~MagnatuneRedownloadDialog()
{
}

void MagnatuneRedownloadDialog::setRedownloadItems( const QStringList &items )
{

     QStringListIterator it(items);
     while ( it.hasNext() ) {

           QString currentItem = it.next();
           debug() << "Adding item to redownload dialog: " << currentItem;
           redownloadListView->addTopLevelItem( new QTreeWidgetItem( QStringList(currentItem) ) );
     }

     debug() << "Nothing more to add...";

}

void MagnatuneRedownloadDialog::setRedownloadItems( QList<MagnatuneDownloadInfo> previousPurchases )
{
    m_infoMap.clear();
    
    foreach( MagnatuneDownloadInfo prevPurchase, previousPurchases )
    {
        QString albumText = prevPurchase.artistName() + " - " +  prevPurchase.albumName();
        QTreeWidgetItem * item = new QTreeWidgetItem( QStringList( albumText ) );
        m_infoMap.insert( item, prevPurchase );
        redownloadListView->addTopLevelItem( item );
    }

    
}

void MagnatuneRedownloadDialog::redownload( )
{

    QTreeWidgetItem * current = redownloadListView->currentItem();
    if ( m_infoMap.keys().contains( current ) )
    {
        emit( redownload( m_infoMap.value( current ) ) );
    }
    
    //emit ( redownload( redownloadListView->currentItem()->text( 0 ) ) );

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


#include "MagnatuneRedownloadDialog.moc"

