/****************************************************************************************
 * Copyright (c) 2008 William Viana Soares <vianasw@gmail.com>                          *
 * Copyright (c) 2008 Seb Ruiz <ruiz@kde.org>                                           *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "AlbumsView.h"

#include "AlbumItem.h"
#include "Debug.h"
#include "SvgHandler.h"
#include "TrackItem.h"
#include "dialogs/TagDialog.h"
#include "meta/capabilities/CustomActionsCapability.h"
#include "playlist/PlaylistController.h"
#include "widgets/PrettyTreeView.h"

#include <QGraphicsSceneContextMenuEvent>
#include <QHeaderView>
#include <QTreeView>

#include <KAction>
#include <KIcon>
#include <KMenu>

// Subclassed to override the access level of some methods.
// The AlbumsTreeView and the AlbumsView are so highly coupled that this is acceptable, imo.
class AlbumsTreeView : public Amarok::PrettyTreeView
{
    public:
        AlbumsTreeView( QWidget *parent = 0 )
            : Amarok::PrettyTreeView( parent )
        {
            setAttribute( Qt::WA_NoSystemBackground );
            viewport()->setAutoFillBackground( false );

            setHeaderHidden( true );
            setIconSize( QSize(60,60) );
            setDragDropMode( QAbstractItemView::DragOnly );
            setSelectionMode( QAbstractItemView::ExtendedSelection );
            setSelectionBehavior( QAbstractItemView::SelectItems );
            //setAnimated( true ); // looks TERRIBLE
            
            setRootIsDecorated( false );

            setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
            setVerticalScrollMode( QAbstractItemView::ScrollPerPixel ); // Scrolling per item is really not smooth and looks terrible
        }

        // Override access level to make it public. Only visible to the AlbumsView.
        // Used for context menu methods.
        QModelIndexList selectedIndexes() const { return PrettyTreeView::selectedIndexes(); }
};

AlbumsView::AlbumsView( QGraphicsWidget *parent )
    : QGraphicsProxyWidget( parent )
{
    AlbumsTreeView* treeView = new AlbumsTreeView;
    setWidget( treeView );
    
    connect( treeView, SIGNAL(       clicked( const QModelIndex & ) ), this, SLOT( itemClicked( const QModelIndex & ) ) );
    connect( treeView, SIGNAL( doubleClicked( const QModelIndex & ) ), this, SLOT( slotAppendSelected() ) );

    treeView->show();
}

AlbumsView::~AlbumsView()
{
}

void
AlbumsView::setModel( QAbstractItemModel *model )
{
    nativeWidget()->setModel( model );                                                                                               
}

QAbstractItemModel*
AlbumsView::model()
{
    return nativeWidget()->model();
}

QTreeView*
AlbumsView::nativeWidget() const
{
    return static_cast<QTreeView*>( widget() );
}

void
AlbumsView::itemClicked( const QModelIndex &index )
{
    bool expanded = nativeWidget()->isExpanded( index );    
    nativeWidget()->setExpanded( index, !expanded );
}

void
AlbumsView::contextMenuEvent( QGraphicsSceneContextMenuEvent *event )
{
    KAction *appendAction = new KAction( KIcon( "media-track-add-amarok" ), i18n( "&Append to Playlist" ), this );
    KAction *loadAction   = new KAction( KIcon( "folder-open" ), i18nc( "Replace the currently loaded tracks with these", "&Load" ), this );
    KAction *queueAction  = new KAction( KIcon( "media-track-queue-amarok" ), i18n( "&Queue" ), this );
    KAction *editAction   = new KAction( KIcon( "media-track-edit-amarok" ), i18n( "Edit Track Details" ), this );
    
    connect( appendAction, SIGNAL( triggered() ), this, SLOT( slotAppendSelected() ) );
    connect( loadAction  , SIGNAL( triggered() ), this, SLOT( slotPlaySelected() ) );
    connect( queueAction , SIGNAL( triggered() ), this, SLOT( slotQueueSelected() ) );
    connect( editAction  , SIGNAL( triggered() ), this, SLOT( slotEditSelected() ) );

    KMenu menu;
    menu.addAction( appendAction );
    menu.addAction( loadAction );
    menu.addAction( queueAction );
    menu.addSeparator();
    menu.addAction( editAction );

    QModelIndex index = nativeWidget()->indexAt( event->pos().toPoint() );
    if( index.isValid() )
    {
        QStandardItem *item = static_cast<QStandardItemModel*>( model() )->itemFromIndex( index );
        AlbumItem *album = dynamic_cast<AlbumItem*>(item);
        if( album )
        {
            Meta::AlbumPtr albumPtr = album->album();
            Meta::CustomActionsCapability *cac = albumPtr->create<Meta::CustomActionsCapability>();
            if( cac )
            {
                QList<QAction *> actions = cac->customActions();

                menu.addSeparator();
                foreach( QAction *action, actions )
                    menu.addAction( action );
            }
        }
    }

    menu.exec( event->screenPos() );
}

void
AlbumsView::slotAppendSelected()
{
    Meta::TrackList selected = getSelectedTracks();
    The::playlistController()->insertOptioned( selected, Playlist::AppendAndPlay );
}

void
AlbumsView::slotPlaySelected()
{
    Meta::TrackList selected = getSelectedTracks();
    The::playlistController()->insertOptioned( selected, Playlist::LoadAndPlay );
}

void
AlbumsView::slotQueueSelected()
{
    Meta::TrackList selected = getSelectedTracks();
    The::playlistController()->insertOptioned( selected, Playlist::Queue );
}

void
AlbumsView::slotEditSelected()
{
    Meta::TrackList selected = getSelectedTracks();
    if( !selected.isEmpty() )
    {
        TagDialog *dialog = new TagDialog( selected );
        dialog->show();
    }
}

Meta::TrackList
AlbumsView::getSelectedTracks() const
{
    Meta::TrackList selected;

    const QStandardItemModel *itemModel = static_cast<QStandardItemModel*>( const_cast<AlbumsView*>(this)->model());
    QModelIndexList indexes = static_cast<AlbumsTreeView*>(nativeWidget())->selectedIndexes();

    foreach( const QModelIndex &index, indexes )
    {
        if( index.isValid() )
        {
            QStandardItem *item = itemModel->itemFromIndex( index );
            AlbumItem *album = dynamic_cast<AlbumItem*>(item);
            if( album )
            {
                selected << album->album()->tracks();
                continue;
            }
            TrackItem *track = dynamic_cast<TrackItem*>(item);
            if( track )
                selected << track->track();
        }
    }

    return selected;
}

void
AlbumsView::resizeEvent( QGraphicsSceneResizeEvent *event )
{
    QGraphicsProxyWidget::resizeEvent( event );

    const int newWidth = size().width() / nativeWidget()->header()->count();

    for( int i = 0; i < nativeWidget()->header()->count(); ++i )
        nativeWidget()->header()->resizeSection( i, newWidth );

    nativeWidget()->setColumnWidth( 0, 100 );
}

#include <AlbumsView.moc>

