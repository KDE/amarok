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

#include "debug.h"
#include "PlaylistModel.h"
#include "tagdialog.h"
#include "meta/CurrentTrackActionsCapability.h"

#include <KAction>
#include <KMenu>

#include <QHeaderView>
#include <QModelIndex>
#include <QKeyEvent>



Playlist::ClassicView::ClassicView(QWidget * parent)
    : QTreeView( parent )
{
    DEBUG_BLOCK

    setUniformRowHeights(true);
    header()->setMovable(true);
    header()->setClickable(true);
    setDragEnabled(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::InternalMove);

}

Playlist::ClassicView::~ ClassicView()
{
}



void Playlist::ClassicView::setModel( Playlist::Model *model )
{
    DEBUG_BLOCK

    m_model = model;
    connect ( this, SIGNAL( activated ( const QModelIndex & ) ), m_model , SLOT( play(const QModelIndex & ) ) );   

    QTreeView::setModel( model );

    //just for fun
    header()->hideSection(0);
    header()->hideSection(1);
    header()->hideSection(2);

}

void Playlist::ClassicView::playContext()
{
    DEBUG_BLOCK

        if(!m_contextIndex)
            return;

    m_model->play( m_contextIndex->row() );
    m_contextIndex = 0;

}

void Playlist::ClassicView::contextMenuEvent( QContextMenuEvent *event )
{
    DEBUG_BLOCK
    QModelIndex index = indexAt(event->pos());
    if( !index.isValid() )
        return;

    m_contextIndex = new QPersistentModelIndex(index);
    //item->setSelected( true );
    event->accept();

    KAction *playAction = new KAction( KIcon( "media-playback-start-amarok" ), i18n( "&Play" ), this );
    //playAction->setData( QVariant( sceneClickPos ) );
    connect( playAction, SIGNAL( triggered() ), this, SLOT( playContext() ) );

    KMenu *menu = new KMenu( this );
    
    menu->addAction( playAction );
    ( menu->addAction( i18n( "Queue Track" ), this, SLOT( queueItem() ) ) )->setEnabled( false );
    ( menu->addAction( i18n( "Stop Playing After Track" ), this, SLOT( stopAfterTrack() ) ) )->setEnabled( false );
    menu->addSeparator();
    ( menu->addAction( i18n( "Remove From Playlist" ), this, SLOT( removeSelection() ) ) )->setEnabled( true );
    menu->addSeparator();
    menu->addAction( i18n( "Edit Track Information" ), this, SLOT( editTrackInformation() ) );
    menu->addSeparator();



    //Meta::TrackPtr  item = m_model->data(index, TrackRole).value< Meta::TrackPtr >();
    //lets see if this is the currently playing tracks, and if it has CurrentTrackActionsCapability
    if( index.data( ActiveTrackRole ).toBool() ) {

        if ( index.data( ItemRole ).value< Playlist::Item* >()->track()->hasCapabilityInterface( Meta::Capability::CurrentTrackActions ) ) {
            debug() << "2";
            Meta::CurrentTrackActionsCapability *cac = index.data( ItemRole ).value< Playlist::Item* >()->track()->as<Meta::CurrentTrackActionsCapability>();
            if( cac )
            {
                QList<QAction *> actions = cac->customActions();

                foreach( QAction *action, actions )
                    menu->addAction( action );
                menu->addSeparator();
            }
        }
    }
    menu->exec( event->globalPos() );
}

void Playlist::ClassicView::removeSelection()
{
    DEBUG_BLOCK

    QModelIndexList indexes = selectionModel()->selectedRows() ;
    while ( indexes.size() > 0 ) {

        QModelIndex i = indexes.takeFirst();
        int count = 1;
        while (!indexes.isEmpty() && indexes.takeFirst().row() == i.row() + count)
            ++count;
        model()->removeRows( i.row(), count, i.parent() );
        indexes = selectionModel()->selectedRows() ;
    }
}

void Playlist::ClassicView::editTrackInformation()
{
    DEBUG_BLOCK
    QModelIndexList selected = selectedIndexes();
    Meta::TrackList tracks;
    foreach(QModelIndex i, selected)
    {
        tracks << m_model->data(i, TrackRole).value< Meta::TrackPtr >();
    }

        if(!m_contextIndex)
            return;

    TagDialog *dialog = new TagDialog( tracks, this );
    dialog->show();
    m_contextIndex = 0;
}

#include "PlaylistClassicView.moc"




