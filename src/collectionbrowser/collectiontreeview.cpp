 /*
  Copyright (c) 2007  Alexandre Pereira de Oliveira <aleprj@gmail.com>

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

#include "collectiontreeview.h"

#include "amarok.h"
#include "collectionbrowser/collectiontreeitemmodel.h"
#include "collectionbrowser/collectiontreeitem.h"

#include <QSortFilterProxyModel>

#include <kconfig.h>
#include <KSharedPtr>


CollectionTreeView::CollectionTreeView( QWidget *parent)
    : QTreeView( parent )
{
    KConfigGroup config = Amarok::config( "Collection Browser" );
    QList<int> cats = config.readEntry( "TreeCategory", QList<int>() );
    if ( cats.isEmpty() )
        cats << QueryBuilder::tabArtist << QueryBuilder::tabAlbum;

    m_treeModel = new CollectionTreeItemModel( cats );

    m_filterModel = new QSortFilterProxyModel( this );
    m_filterModel->setSortRole( CustomRoles::SortRole );
    m_filterModel->setFilterRole( CustomRoles::FilterRole );
    m_filterModel->setSortCaseSensitivity( Qt::CaseInsensitive );
    m_filterModel->setFilterCaseSensitivity( Qt::CaseInsensitive );
    m_filterModel->setSourceModel( m_treeModel );

    setModel( m_filterModel );
    setSortingEnabled( true );
    sortByColumn( 0, Qt::AscendingOrder );
    setSelectionMode(QAbstractItemView::ExtendedSelection);

    setDragDropMode( QAbstractItemView::DragOnly ); // implement drop when time allows

    //setAnimated( true );
    setAlternatingRowColors( true );
}

CollectionTreeView::~CollectionTreeView() {
    KConfigGroup config = Amarok::config( "Collection Browser" );
    config.writeEntry( "TreeCategory", m_treeModel->levels() );
    //delete m_treeModel;
    //delete m_filterModel;
}

void
CollectionTreeView::setLevels( const QList<int> &levels ) {
    m_treeModel->setLevels( levels );
}


void
CollectionTreeView::setLevel( int level, int type ) {
    QList<int> levels = m_treeModel->levels();
    if ( type == CategoryId::None ) {
        while( levels.count() >= level )
            levels.removeLast();
    }
    else {
        levels.removeAll( type );
        levels[level] = type;
    }
    setLevels( levels );
}

