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

#include "AlbumsView.h"
#include "Debug.h"
#include "context/popupdropper/PopupDropperAction.h"
#include "SvgHandler.h"

#include <QGraphicsSceneContextMenuEvent>
#include <QHeaderView>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QTreeView>

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
            // setAnimated( true ); // looks TERRIBLE

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


        void startDrag(Qt::DropActions supportedActions)
        {
            DEBUG_BLOCK
            QTreeView::startDrag( supportedActions );
            debug() << "After the drag!";
        }
        
};

AlbumsView::AlbumsView( QGraphicsWidget *parent )
    : QGraphicsProxyWidget( parent )
{
    AlbumsTreeView* treeView = new AlbumsTreeView;
    setWidget( treeView );
    
    connect( treeView, SIGNAL(       clicked( const QModelIndex & ) ), this, SLOT( itemClicked( const QModelIndex & ) ) );
    connect( treeView, SIGNAL( doubleClicked( const QModelIndex & ) ), this, SLOT( itemClicked( const QModelIndex & ) ) );

    treeView->show();
}

void
AlbumsView::setModel( QAbstractItemModel *model )
{
    nativeWidget()->setModel( model );                                                                                               
}

QAbstractItemModel *
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
    PopupDropperAction *appendAction = new PopupDropperAction( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ), 
                                           "append", KIcon( "media-track-add-amarok" ),  i18n( "&Append to Playlist" ), this );

    PopupDropperAction *loadAction = new PopupDropperAction( The::svgHandler()->getRenderer( "amarok/images/pud_items.svg" ), 
                                           "load", KIcon( "folder-open" ), i18nc( "Replace the currently loaded tracks with these", "&Load" ), this );
    
    connect( appendAction, SIGNAL( triggered() ), this, SLOT( slotAppendSelected() ) );
    connect( loadAction  , SIGNAL( triggered() ), this, SLOT( slotPlaySelected() ) );

    KMenu menu;
    menu.addAction( appendAction );
    menu.addAction( loadAction );

    menu.exec( event->screenPos() );
}

void
AlbumsView::slotAppendSelected()
{
    Meta::TrackList selected = getSelectedTracks();
}

void
AlbumsView::slotPlaySelected()
{
    Meta::TrackList selected = getSelectedTracks();
}

Meta::TrackList
AlbumsView::getSelectedTracks() const
{
    Meta::TrackList selected;
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

