/****************************************************************************************
 * Copyright (c) 2006,2007 Nikolaj Hald Nielsen <nhn@kde.org>                           *
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

#include "core/support/Debug.h"

#include <QHeaderView>

MagnatuneRedownloadDialog::MagnatuneRedownloadDialog(QWidget* parent, const char* name, bool modal, Qt::WindowFlags fl)
    : QDialog(parent, fl)
{
    setObjectName( QLatin1String(name) );
    setModal( modal );
    setupUi(this);
    redownloadButton->setEnabled ( false );

    redownloadListView->header()->setStretchLastSection( true );
    redownloadListView->setRootIsDecorated( false );
    connect( redownloadListView, &QTreeWidget::itemSelectionChanged, this, &MagnatuneRedownloadDialog::selectionChanged );
}

MagnatuneRedownloadDialog::~MagnatuneRedownloadDialog()
{
}

void MagnatuneRedownloadDialog::setRedownloadItems( const QStringList &items )
{

     QStringListIterator it(items);
     while ( it.hasNext() ) {

           const QString currentItem = it.next();
           debug() << "Adding item to redownload dialog: " << currentItem;
           redownloadListView->addTopLevelItem( new QTreeWidgetItem( QStringList(currentItem) ) );
     }

     debug() << "Nothing more to add...";

}

void MagnatuneRedownloadDialog::setRedownloadItems( const QList<MagnatuneDownloadInfo> &previousPurchases )
{
    m_infoMap.clear();
    
    for( const MagnatuneDownloadInfo &prevPurchase : previousPurchases )
    {
        const QString albumText = prevPurchase.artistName() + QStringLiteral(" - ") +  prevPurchase.albumName();
        QTreeWidgetItem *item = new QTreeWidgetItem( QStringList( albumText ) );
        m_infoMap.insert( item, prevPurchase );
        redownloadListView->addTopLevelItem( item );
    }

    
}

void MagnatuneRedownloadDialog::slotRedownload( )
{
    QTreeWidgetItem * current = redownloadListView->currentItem();
    if ( m_infoMap.keys().contains( current ) )
    {
        Q_EMIT( redownload( m_infoMap.value( current ) ) );
    }
    
    //Q_EMIT ( redownload( redownloadListView->currentItem()->text( 0 ) ) );

    hide();
}

void MagnatuneRedownloadDialog::reject( )
{
    hide();
    Q_EMIT(cancelled());
}

void MagnatuneRedownloadDialog::selectionChanged( )
{
    if (redownloadListView->currentItem()) {
        redownloadButton->setEnabled ( true );
    } else { 
        redownloadButton->setEnabled ( false );
    }
}

/*$SPECIALIZATION$*/



