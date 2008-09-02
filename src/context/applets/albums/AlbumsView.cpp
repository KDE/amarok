/*******************************************************************************
* copyright              : (C) 2008 William Viana Soares <vianasw@gmail.com>   *
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

#include <QTreeView>
#include <QHeaderView>
#include <QItemDelegate>

#include <KIconLoader>

#include "AlbumsDelegate.h"


AlbumsView::AlbumsView( QGraphicsWidget *parent )
    : QGraphicsProxyWidget( parent )
{
    QTreeView* native = new QTreeView;
    setWidget( native );
    native->setAttribute( Qt::WA_NoSystemBackground );
    native->viewport()->setAutoFillBackground( false );
    native->setFrameStyle( QFrame::NoFrame );
    native->setHeaderHidden( true );

    // native->setAnimated( true ); // causes flickering

    native->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    native->setIconSize( QSize( KIconLoader::SizeSmallMedium, KIconLoader::SizeSmallMedium ) );
    native->setRootIsDecorated( true );
    native->setMouseTracking( true );
    native->setVerticalScrollMode( QAbstractItemView::ScrollPerPixel ); // Scrolling per item is really not smooth and looks terrible

    AlbumsDelegate *delegate = new AlbumsDelegate( native );
    native->setItemDelegate( delegate );
    connect( native, SIGNAL(       clicked( const QModelIndex & ) ), this, SLOT( itemClicked( const QModelIndex & ) ) );
    connect( native, SIGNAL( doubleClicked( const QModelIndex & ) ), this, SLOT( itemClicked( const QModelIndex & ) ) );
//     connect( native, SIGNAL( entered( const QModelIndex & ) ), delegate, SLOT( highlightRow( const QModelIndex & ) ) );

    native->show();
}

AlbumsView::~AlbumsView()
{
}

void
AlbumsView::itemClicked( const QModelIndex &index )
{
    bool expanded = nativeWidget()->isExpanded( index );    
    nativeWidget()->setExpanded( index, !expanded );
}

/*
 * Disabled until we can reference an album/track by their internal id instead of their names.
void
AlbumsView::itemDoubleClicked( const QModelIndex &index )
{
    DEBUG_BLOCK
    if( index.data( AlbumRoles::AlbumName ) != QVariant() )
    {
        emit enqueueAlbum( index.data( AlbumRoles::AlbumName ).toString() );
    }
    else
    {
        emit enqueueTrack( index.parent().data( AlbumRoles::AlbumName ).toString(),
                           index.data( AlbumRoles::TrackName ).toString() );
    }
}
*/

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

void
AlbumsView::setStyleSheet( const QString &stylesheet )
{
    widget()->setStyleSheet( stylesheet );
}

QString
AlbumsView::styleSheet()
{
    return widget()->styleSheet();
}

QTreeView*
AlbumsView::nativeWidget() const
{
    return static_cast<QTreeView*>( widget() );
}

void
AlbumsView::resizeEvent( QGraphicsSceneResizeEvent *event )
{
    QGraphicsProxyWidget::resizeEvent( event );

    const int newWidth = size().width() / nativeWidget()->header()->count();

    for ( int i = 0; i < nativeWidget()->header()->count(); ++i ) {
        nativeWidget()->header()->resizeSection( i, newWidth );
    }
    nativeWidget()->setColumnWidth( 0, 100 );
}

#include <AlbumsView.moc>

