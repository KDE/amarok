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
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#include "AlbumsView.h"

#include "AlbumItem.h"
#include "AlbumsDefs.h"
#include "SvgHandler.h"
#include "TrackItem.h"
#include "dialogs/TagDialog.h"
#include "core/capabilities/CustomActionsCapability.h"
#include "playlist/PlaylistModelStack.h"
#include "widgets/PrettyTreeView.h"

#include <KAction>
#include <KGlobalSettings>
#include <KIcon>
#include <KMenu>

#include <QApplication>
#include <QGraphicsSceneContextMenuEvent>
#include <QHeaderView>
#include <QPainter>
#include <QTreeView>

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
            if( KGlobalSettings::graphicEffectsLevel() != KGlobalSettings::NoEffects )
                setAnimated( true );
            setRootIsDecorated( false );
            setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
            setVerticalScrollMode( QAbstractItemView::ScrollPerPixel ); // Scrolling per item is really not smooth and looks terrible
            setItemDelegate( new AlbumsItemDelegate( this ) );
        }

        // Override access level to make it public. Only visible to the AlbumsView.
        // Used for context menu methods.
        QModelIndexList selectedIndexes() const { return PrettyTreeView::selectedIndexes(); }
};

AlbumsView::AlbumsView( QGraphicsWidget *parent )
    : QGraphicsProxyWidget( parent )
{
    AlbumsTreeView *treeView = new AlbumsTreeView( 0 );
    connect( treeView, SIGNAL(clicked(QModelIndex)), this, SLOT(itemClicked(QModelIndex)) );
    connect( treeView, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(slotAppendSelected()) );
    setWidget( treeView );
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
AlbumsView::model() const
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
    if( expanded )
        nativeWidget()->setExpanded( index, !expanded );
    else
        setRecursiveExpanded( index, !expanded );
}

void
AlbumsView::contextMenuEvent( QGraphicsSceneContextMenuEvent *event )
{
    const QModelIndex index = nativeWidget()->indexAt( event->pos().toPoint() );
    if( !index.isValid() )
    {
        QGraphicsProxyWidget::contextMenuEvent( event );
        return;
    }

    KMenu menu;
    KAction *appendAction = new KAction( KIcon( "media-track-add-amarok" ), i18n( "&Add to Playlist" ), &menu );
    KAction *loadAction   = new KAction( KIcon( "folder-open" ), i18nc( "Replace the currently loaded tracks with these", "&Replace Playlist" ), &menu );
    KAction *queueAction  = new KAction( KIcon( "media-track-queue-amarok" ), i18n( "&Queue" ), &menu );
    KAction *editAction   = new KAction( KIcon( "media-track-edit-amarok" ), i18n( "Edit Track Details" ), &menu );

    menu.addAction( appendAction );
    menu.addAction( loadAction );
    menu.addAction( queueAction );
    menu.addAction( editAction );

    connect( appendAction, SIGNAL(triggered()), this, SLOT(slotAppendSelected()) );
    connect( loadAction  , SIGNAL(triggered()), this, SLOT(slotPlaySelected()) );
    connect( queueAction , SIGNAL(triggered()), this, SLOT(slotQueueSelected()) );
    connect( editAction  , SIGNAL(triggered()), this, SLOT(slotEditSelected()) );

    KMenu menuCover( i18n( "Album" ), &menu );
    QStandardItem *item = static_cast<QStandardItemModel*>( model() )->itemFromIndex( index );
    AlbumItem *album = dynamic_cast<AlbumItem*>(item);
    if( album )
    {
        Meta::AlbumPtr albumPtr = album->album();
        Capabilities::CustomActionsCapability *cac = albumPtr->create<Capabilities::CustomActionsCapability>();
        if( cac )
        {
            QList<QAction *> actions = cac->customActions();
            if( !actions.isEmpty() )
            {
                menuCover.addActions( actions );
                menuCover.setIcon( KIcon( "filename-album-amarok" ) );
                menu.addMenu( &menuCover );
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
AlbumsView::setRecursiveExpanded( const QModelIndex &index, bool expanded )
{
    if( model()->hasChildren( index ) )
    {
        for( int i = 0, count = model()->rowCount( index ); i < count; ++i )
            nativeWidget()->setExpanded( index.child( i, 0 ), expanded );
    }
    nativeWidget()->setExpanded( index, expanded );
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

/*
 * Customize the painting of AlbumItems in the tree view. The album items'
 * displayed text is like so: "artist - album (year)\ntracks, time", i.e. the
 * album info is shown in two lines. When resizing the context view, the string
 * is elided if the available width is less than the text width. That ellipsis
 * is done on the whole string however, so the second line disappears. A
 * solution is to do the eliding ourselves.
 */
void
AlbumsItemDelegate::paint( QPainter *p,
                           const QStyleOptionViewItem &option,
                           const QModelIndex &index ) const
{
    QStyledItemDelegate::paint( p, option, index );
    const QStandardItemModel *itemModel = static_cast<const QStandardItemModel *>( index.model() );
    const QStandardItem *item = itemModel->itemFromIndex( index );
    if( item->type() == AlbumType )
    {
        // draw the text ourselves. The superclass will skip painting the
        // text since the text in Qt::DisplayRole is not actually set.
        const QString &text = index.data( AlbumDisplayRole ).toString();
        if( !text.isEmpty() )
        {
            QStyleOptionViewItemV4 vopt( option );
            initStyleOption( &vopt, index );
            const AlbumItem *albumItem = static_cast<const AlbumItem *>( item );
            int iconOffset = albumItem->iconSize() + 6; // 6 is from the added svg borders
            vopt.rect.adjust( iconOffset, 0, 0, 0 );
            vopt.text = text;
            drawAlbumText( p, vopt );
        }
    }
}

void
AlbumsItemDelegate::drawAlbumText( QPainter *p, const QStyleOptionViewItemV4 &vopt ) const
{
    const QRect &textRect = vopt.rect.adjusted( 2, 0, -4, 0 );

    p->save();
    p->setClipRect( textRect );
    applyCommonStyle( p, vopt );

    // elide each line according to available width
    QFontMetrics fm = vopt.fontMetrics;
    QStringList texts = vopt.text.split('\n');
    QMutableStringListIterator it( texts );
    while( it.hasNext() )
    {
        const QString &text = it.next();
        if( fm.width( text ) > textRect.width() )
            it.setValue( fm.elidedText( text, Qt::ElideRight, textRect.width() ) );
    }

    p->drawText( textRect, Qt::AlignLeft | Qt::AlignVCenter, texts.join("\n") );
    p->restore();
}

void
AlbumsItemDelegate::applyCommonStyle( QPainter *p, const QStyleOptionViewItemV4 &vopt ) const
{
    // styling code from QCommonStyle. These aren't actually used right
    // now, but will be needed if something like inline tag editing is
    // implemented.
    QPalette::ColorGroup cg = vopt.state & QStyle::State_Enabled
        ? QPalette::Normal : QPalette::Disabled;
    if (cg == QPalette::Normal && !(vopt.state & QStyle::State_Active))
        cg = QPalette::Inactive;

    if (vopt.state & QStyle::State_Selected) {
        p->setPen(vopt.palette.color(cg, QPalette::HighlightedText));
    } else {
        p->setPen(vopt.palette.color(cg, QPalette::Text));
    }
    if (vopt.state & QStyle::State_Editing) {
        p->setPen(vopt.palette.color(cg, QPalette::Text));
        p->drawRect(vopt.rect.adjusted(0, 0, -1, -1));
    }
}

#include <AlbumsView.moc>
