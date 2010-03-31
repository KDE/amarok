/******************************************************************************
 * Copyright (c) 2007 Alexandre Pereira de Oliveira <aleprj@gmail.com>        *
 *           (c) 2007 Ian Alexander Monroe <ian@monroe.nu>                    *
 *           (c) 2009 Soren Harward <stharward@gmail.com>                     *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License as             *
 * published by the Free Software Foundation; either version 2 of             *
 * the License or (at your option) version 3 or any later version             *
 * accepted by the membership of KDE e.V. (or its successor approved          *
 * by the membership of KDE e.V.), which shall act as a proxy                 *
 * defined in Section 14 of version 3 of the license.                         *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 ******************************************************************************/

#include "CollectionTreeViewSimple.h"

#include "CollectionTreeItemModel.h"

#include "core/support/Debug.h"

#include <QSet>

CollectionTreeViewSimple::CollectionTreeViewSimple( QWidget *parent) : Amarok::PrettyTreeView( parent )
    , m_model( 0 )
{
    setSortingEnabled( true );
    sortByColumn( 0, Qt::AscendingOrder );
    setSelectionMode( QAbstractItemView::SingleSelection );
    setSelectionBehavior( QAbstractItemView::SelectRows );
#ifdef Q_WS_MAC
    setVerticalScrollMode( QAbstractItemView::ScrollPerItem ); // for some bizarre reason w/ some styles on mac
    setHorizontalScrollMode( QAbstractItemView::ScrollPerItem ); // per-pixel scrolling is slower than per-item
#else
    setVerticalScrollMode( QAbstractItemView::ScrollPerPixel ); // Scrolling per item is really not smooth and looks terrible
    setHorizontalScrollMode( QAbstractItemView::ScrollPerPixel ); // Scrolling per item is really not smooth and looks terrible
#endif
    
    // Runtime check for Qt 4.5 here. Older versions produce graphical garbage with animation enabled.
    const QChar major = qVersion()[0];
    const QChar minor = qVersion()[2];
    if( major.digitValue() >= 4 && minor.digitValue() >= 5 ) 
        setAnimated( true );

    setStyleSheet("QTreeView::item { margin-top: 1px; margin-bottom: 1px; }"); //ensure a bit of space around the cover icons

    connect( this, SIGNAL( collapsed( const QModelIndex & ) ), SLOT( slotCollapsed( const QModelIndex & ) ) );
    connect( this, SIGNAL( expanded( const QModelIndex & ) ), SLOT( slotExpanded( const QModelIndex & ) ) );
}

CollectionTreeViewSimple::~CollectionTreeViewSimple()
{
    DEBUG_BLOCK

    delete m_model;
}

void
CollectionTreeViewSimple::setModel( QAbstractItemModel* model )
{
    m_model = qobject_cast<CollectionTreeItemModelBase*>( model );
    if ( !m_model )
        return;

    QTreeView::setModel( m_model );

    connect( m_model, SIGNAL( expandIndex( const QModelIndex& ) ), this, SLOT( slotExpand( const QModelIndex& ) ) );
}

// TODO: more complicated than it needs to be, because of SingleSelection
void
CollectionTreeViewSimple::selectionChanged(const QItemSelection& selected, const QItemSelection& )
{
    QModelIndexList indexes = selected.indexes();
    if ( indexes.count() < 1 )
        return;

    CollectionTreeItem * item = static_cast<CollectionTreeItem *>( indexes.at( 0 ).internalPointer() );

    emit( itemSelected ( item ) );
}

void
CollectionTreeViewSimple::slotExpand( const QModelIndex &index )
{
    expand( index );
}

void
CollectionTreeViewSimple::slotCollapsed( const QModelIndex &index )
{
    m_model->slotCollapsed( index );
}

void
CollectionTreeViewSimple::slotExpanded( const QModelIndex &index )
{
    m_model->slotExpanded( index );
}
