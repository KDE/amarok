/***************************************************************************
 *   Copyright (c) 2007  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#include "PlaylistClassicView.h"

#include "Debug.h"
#include "TagDialog.h"
#include "meta/CurrentTrackActionsCapability.h"
#include "playlist/PlaylistActions.h"
#include "playlist/PlaylistModel.h"
#include "playlist/view/PlaylistViewCommon.h"

#include <KAction>
#include <KMenu>

#include <QHeaderView>
#include <QModelIndex>
#include <QKeyEvent>



Playlist::ClassicView::ClassicView(QWidget * parent)
    : QWidget( parent ), 
    m_treeView( new QTreeView( this )),
    m_layout( new QVBoxLayout( this )),
    m_lineEdit( new KLineEdit( this )),
    m_proxyModel( new QSortFilterProxyModel )
{
    DEBUG_BLOCK
    setLayout(m_layout);

    m_treeView->setUniformRowHeights(true);
    m_treeView->setSortingEnabled(true);
    m_treeView->header()->setMovable(true);
    m_treeView->header()->setClickable(true);
    m_treeView->setDragEnabled(true);
    m_treeView->setDropIndicatorShown(true);
    m_treeView->setDragDropMode(QAbstractItemView::InternalMove);
    m_treeView->setRootIsDecorated( false );
    m_treeView->setAlternatingRowColors ( true );
    m_treeView->setSelectionMode( QAbstractItemView::ExtendedSelection );
    //just for fun

    m_model = The::playlistModel();
    //Might need to adjust this for the proxy model
    connect ( m_treeView, SIGNAL( activated ( const QModelIndex & ) ), this , SLOT( playTrack(const QModelIndex & ) ) );   

    m_proxyModel->setSourceModel( m_model );
    m_treeView->setModel( m_proxyModel );


    connect(m_lineEdit, SIGNAL(textChanged(const QString &)),
            m_proxyModel, SLOT(setFilterRegExp(const QString &)));
    m_proxyModel->setFilterKeyColumn( -1 );
    m_proxyModel->setFilterCaseSensitivity( Qt::CaseInsensitive );
    m_proxyModel->setSortCaseSensitivity( Qt::CaseInsensitive );


    m_layout->addWidget( m_lineEdit );
    m_layout->addWidget( m_treeView );

    m_treeView->header()->hideSection(0);
    m_treeView->header()->hideSection(1);
    m_treeView->header()->hideSection(2);

}

Playlist::ClassicView::~ ClassicView()
{
}

void Playlist::ClassicView::playTrack(const QModelIndex &index)
{
    The::playlistActions()->play( m_proxyModel->mapToSource( index ).row() );
}

void Playlist::ClassicView::playTrack()
{
    DEBUG_BLOCK

        if(!m_contextIndex)
            return;

    The::playlistActions()->play( m_proxyModel->mapToSource( *m_contextIndex ).row() );
    m_contextIndex = 0;

}

void Playlist::ClassicView::contextMenuEvent( QContextMenuEvent *event )
{
    DEBUG_BLOCK
    QModelIndex index = m_treeView->indexAt(event->pos());
    if( !index.isValid() )
        return;

    m_contextIndex = new QPersistentModelIndex(index);
    //item->setSelected( true );
    event->accept();

    ViewCommon::trackMenu(this, &index, event->globalPos());

}

void Playlist::ClassicView::removeSelection()
{
    DEBUG_BLOCK

    QModelIndexList indexes = m_treeView->selectionModel()->selectedRows() ;
    while ( indexes.size() > 0 ) {

        QModelIndex i = indexes.takeFirst();
        int count = 1;
        while (!indexes.isEmpty() && indexes.takeFirst().row() == i.row() + count)
            ++count;
        m_treeView->model()->removeRows( i.row(), count, i.parent() );
        indexes = m_treeView->selectionModel()->selectedRows() ;
    }
}

void Playlist::ClassicView::editTrackInformation()
{
    DEBUG_BLOCK
        /*
    QModelIndexList selected = m_treeView->selectedIndexes();
    Meta::TrackList tracks;
    foreach(QModelIndex i, selected)
    {
        tracks << m_model->data(i, TrackRole).value< Meta::TrackPtr >();
    }

    TagDialog *dialog = new TagDialog( tracks, this );
    dialog->show();
    */
}
