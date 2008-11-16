/*******************************************************************************
* copyright              : (C) 2008 William Viana Soares <vianasw@gmail.com>   *
*                        : (C) 2008 Seb Ruiz <ruiz@kde.org>                    *
*                                                                              *
********************************************************************************/

/*******************************************************************************
*                                                                              *
*   This program is free software; you can redistribute it and/or modify       *
*   it under the terms of the GNU General Public License as published by       *
*   the Free Software Foundation; either version 2 of the License, or          *
*   (at your option) any later version.                                        *
*                                                                              *
********************************************************************************/

#include "AlbumItem.h"
#include "AlbumsView.h"
#include "Debug.h"
#include "SvgHandler.h"
#include "TrackItem.h"
#include "context/popupdropper/popupdropper/PopupDropperAction.h"
#include "dialogs/TagDialog.h"
#include "meta/CustomActionsCapability.h"
#include "playlist/PlaylistController.h"

#include <QGraphicsSceneContextMenuEvent>
#include <QHeaderView>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QTreeView>

#include <KAction>
#include <KIcon>
#include <KIconLoader>
#include <KMenu>

class AlbumsTreeView : public QTreeView
{
    public:
        AlbumsTreeView( QWidget * parent = 0 ) : QTreeView( parent )
        {
            setAttribute( Qt::WA_NoSystemBackground );
            viewport()->setAutoFillBackground( false );
    
            setFrameStyle( QFrame::NoFrame );
            setHeaderHidden( true );
            setIconSize( QSize(60,60) );
            setDragDropMode( QAbstractItemView::DragOnly );
            setSelectionMode( QAbstractItemView::ExtendedSelection );
            setSelectionBehavior( QAbstractItemView::SelectItems );
            //setAnimated( true ); // looks TERRIBLE

            setRootIsDecorated( false );
            setMouseTracking( true );
    
            setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
            setVerticalScrollMode( QAbstractItemView::ScrollPerPixel ); // Scrolling per item is really not smooth and looks terrible
            
            setAlternatingRowColors( true );
            //transparency
            QPalette p = palette();
            QColor c = p.color( QPalette::Base );
            c.setAlpha( 0 );
            p.setColor( QPalette::Base, c );
            
            //HACK ALERT, make a workaround, for now, for the alternating row color issue
            c = Qt::white;
            c.setAlpha( 31 );
            p.setColor( QPalette::AlternateBase, c );
            
            setPalette( p );
        }

        // The AlbumsTreeView and the AlbumsView are so highly coupled that this is acceptable, imo.
        // Used for context menu methods.
        friend class AlbumsView;

    protected:
        void drawRow( QPainter *painter, const QStyleOptionViewItem & option, const QModelIndex & index) const
        {
            const QStyleOptionViewItemV4 opt = option;

            const bool alternate = opt.features & QStyleOptionViewItemV2::Alternate;

            const int width = option.rect.width();
            const int height = option.rect.height();

            if( height > 0 )
            {
                painter->save();
                QPixmap background;

                if ( !alternate )
                    background = The::svgHandler()->renderSvgWithDividers( "service_list_item", width, height, "service_list_item" );
                else
                    background = The::svgHandler()->renderSvgWithDividers( "alt_service_list_item", width, height, "alt_service_list_item" );

                painter->drawPixmap( option.rect.topLeft().x(), option.rect.topLeft().y(), background );

                painter->restore();
            }
    
            QTreeView::drawRow( painter, option, index ); 
        }
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
    KAction *loadAction = new KAction( KIcon( "folder-open" ), i18nc( "Replace the currently loaded tracks with these", "&Load" ), this );
    KAction *editAction = new KAction( KIcon( "media-track-edit-amarok" ), i18n( "Edit Track Details" ), this );
    
    connect( appendAction, SIGNAL( triggered() ), this, SLOT( slotAppendSelected() ) );
    connect( loadAction  , SIGNAL( triggered() ), this, SLOT( slotPlaySelected() ) );
    connect( editAction  , SIGNAL( triggered() ), this, SLOT( slotEditSelected() ) );

    KMenu menu;
    menu.addAction( appendAction );
    menu.addAction( loadAction );
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
            Meta::CustomActionsCapability *cac = albumPtr->as<Meta::CustomActionsCapability>();
            if( cac )
            {
                QList<PopupDropperAction *> actions = cac->customActions();

                menu.addSeparator();
                foreach( PopupDropperAction *action, actions )
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

