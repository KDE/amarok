/****************************************************************************************
 * Copyright (c) 2008 Nikolaj Hald Nielsen <nhn@kde.org>                                *
 * Copyright (c) 2010 Bart Cerneels <bart.cerneels@kde.org>                             *
 * Copyright (c) 2011 Ralf Engels <ralf-engels@gmx.de>                                  *
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

#define DEBUG_PREFIX "DynamicView"

#include "DynamicView.h"
#include "DynamicBiasDelegate.h"
#include "DynamicBiasDialog.h"

#include "core/support/Debug.h"
#include "dynamic/Bias.h"
#include "dynamic/DynamicModel.h"
#include "dynamic/DynamicPlaylist.h"
#include "dynamic/biases/SearchQueryBias.h"
#include "playlist/PlaylistActions.h"

#include "PopupDropperFactory.h"
// #include "context/popupdropper/libpud/PopupDropperItem.h"
// #include "context/popupdropper/libpud/PopupDropper.h"

#include "PaletteHandler.h"

#include <QAction>
#include <QMenu>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QModelIndex>
#include <QToolTip>

PlaylistBrowserNS::DynamicView::DynamicView( QWidget *parent )
    : Amarok::PrettyTreeView( parent )
{
    DEBUG_BLOCK
    setHeaderHidden( true );
    setSelectionMode( QAbstractItemView::SingleSelection );
    setModel( Dynamic::DynamicModel::instance() );
    setItemDelegate( new PlaylistBrowserNS::DynamicBiasDelegate(this) );

    setSelectionBehavior( QAbstractItemView::SelectItems );
    setDragDropMode( QAbstractItemView::DragDrop /*| QAbstractItemView::InternalMove*/ );
    setDragEnabled( true );
    setAcceptDrops( true );
    setDropIndicatorShown( true );

    setEditTriggers( QAbstractItemView::SelectedClicked | QAbstractItemView::EditKeyPressed );

    // -- expanding the playlist should expand the whole tree
    connect( this, &DynamicView::expanded,
             this, &DynamicView::expandRecursive );
    connect( this, &DynamicView::collapsed,
             this, &DynamicView::collapseRecursive );
}

PlaylistBrowserNS::DynamicView::~DynamicView()
{
}

void
PlaylistBrowserNS::DynamicView::addPlaylist()
{
    DEBUG_BLOCK;
    QModelIndex newIndex = Dynamic::DynamicModel::instance()->newPlaylist();
    selectionModel()->select( newIndex, QItemSelectionModel::ClearAndSelect );
}

void
PlaylistBrowserNS::DynamicView::addToSelected()
{
    DEBUG_BLOCK;
    QModelIndexList indexes = selectionModel()->selectedIndexes();
    if( indexes.isEmpty() )
        return;

    QModelIndex newIndex = Dynamic::DynamicModel::instance()->insertBias( 0, indexes.first(), Dynamic::BiasPtr( new Dynamic::SearchQueryBias() ) );
    selectionModel()->select( newIndex, QItemSelectionModel::ClearAndSelect );
}

void
PlaylistBrowserNS::DynamicView::cloneSelected()
{
    DEBUG_BLOCK;
    QModelIndexList indexes = selectionModel()->selectedIndexes();
    if( indexes.isEmpty() )
        return;

    QModelIndex newIndex = Dynamic::DynamicModel::instance()->cloneAt( indexes.first() );
    selectionModel()->select( newIndex, QItemSelectionModel::ClearAndSelect );
}

void
PlaylistBrowserNS::DynamicView::editSelected()
{
    DEBUG_BLOCK;

    QModelIndexList indexes = selectionModel()->selectedIndexes();
    if( indexes.isEmpty() )
        return;

    QVariant v = model()->data( indexes.first(), Dynamic::DynamicModel::PlaylistRole );
    if( v.isValid() )
    {
        edit( indexes.first() ); // call the normal editor
        return;
    }

    v = model()->data( indexes.first(), Dynamic::DynamicModel::BiasRole );
    if( v.isValid() )
    {
        Dynamic::AbstractBias* bias = qobject_cast<Dynamic::AbstractBias*>(v.value<QObject*>() );
        PlaylistBrowserNS::BiasDialog dialog( Dynamic::BiasPtr(bias), this);
        dialog.exec();
        return;
    }
}

void
PlaylistBrowserNS::DynamicView::removeSelected()
{
    DEBUG_BLOCK;

    QModelIndexList indexes = selectionModel()->selectedIndexes();

    if( indexes.isEmpty() )
        return;

    Dynamic::DynamicModel::instance()->removeAt( indexes.first() );
}

void
PlaylistBrowserNS::DynamicView::expandRecursive(const QModelIndex &index)
{
    for( int i = 0; i < index.model()->rowCount( index ); i++ )
        expand( index.model()->index( i, 0, index ) );
}

void
PlaylistBrowserNS::DynamicView::collapseRecursive(const QModelIndex &index)
{
    for( int i = 0; i < index.model()->rowCount( index ); i++ )
        collapse( index.model()->index( i, 0, index ) );
}


void
PlaylistBrowserNS::DynamicView::keyPressEvent( QKeyEvent *event )
{
    switch( event->key() )
    {
        case Qt::Key_Delete:
            removeSelected();
            return;
        case Qt::Key_Enter:
        case Qt::Key_Return:
            editSelected();
            return;
    }
    Amarok::PrettyTreeView::keyPressEvent( event );
}

void
PlaylistBrowserNS::DynamicView::mouseDoubleClickEvent( QMouseEvent *event )
{
    QModelIndex index = indexAt( event->pos() );
    if( index.isValid() )
    {
        // if the click was on a playlist
        QVariant v = model()->data( index, Dynamic::DynamicModel::PlaylistRole );
        if( v.isValid() )
        {
            Dynamic::DynamicModel *model = Dynamic::DynamicModel::instance();
            model->setActivePlaylist( model->playlistIndex( qobject_cast<Dynamic::DynamicPlaylist*>(v.value<QObject*>() ) ) );
            The::playlistActions()->enableDynamicMode( true );
            event->accept();
            return;
        }

        // if the click was on a bias
        v = model()->data( index, Dynamic::DynamicModel::BiasRole );
        if( v.isValid() )
        {
            editSelected();
            event->accept();
            return;
        }
    }

    Amarok::PrettyTreeView::mouseDoubleClickEvent( event );
}

void
PlaylistBrowserNS::DynamicView::contextMenuEvent( QContextMenuEvent *event )
{
    QModelIndex index = indexAt( event->pos() );
    if( !index.isValid() )
        return;

    QList<QAction*> actions;

    // if the click was on a playlist
    QVariant v = model()->data( index, Dynamic::DynamicModel::PlaylistRole );
    if( v.isValid() )
    {
        Dynamic::DynamicPlaylist* playlist = qobject_cast<Dynamic::DynamicPlaylist*>(v.value<QObject*>() );
        Q_UNUSED( playlist );
        QAction* action;

        action = new QAction( QIcon::fromTheme( QStringLiteral("document-properties-amarok") ), i18n( "&Rename playlist" ), this );
        connect( action, &QAction::triggered, this, &DynamicView::editSelected );
        actions.append( action );

        action = new QAction( QIcon::fromTheme( QStringLiteral("document-new") ), i18n( "&Add new Bias" ), this );
        connect( action, &QAction::triggered, this, &DynamicView::addToSelected );
        actions.append( action );

        action = new QAction( QIcon::fromTheme( QStringLiteral("edit-copy") ), i18n( "&Clone Playlist" ), this );
        connect( action, &QAction::triggered, this, &DynamicView::cloneSelected );
        actions.append( action );

        action = new QAction( QIcon::fromTheme( QStringLiteral("edit-delete") ), i18n( "&Delete playlist" ), this );
        connect( action, &QAction::triggered, this, &DynamicView::removeSelected );
        actions.append( action );
    }

    // if the click was on a bias
    v = model()->data( index, Dynamic::DynamicModel::BiasRole );
    if( v.isValid() )
    {
        Dynamic::AbstractBias* bias = qobject_cast<Dynamic::AbstractBias*>(v.value<QObject*>() );
        Q_UNUSED( bias );
        Dynamic::AndBias* aBias = qobject_cast<Dynamic::AndBias*>(v.value<QObject*>() );
        QAction* action;

        action = new QAction( QIcon::fromTheme( QStringLiteral("document-properties-amarok") ), i18n( "&Edit bias..." ), this );
        connect( action, &QAction::triggered, this, &DynamicView::editSelected );
        actions.append( action );

        action = new QAction( QIcon::fromTheme( QStringLiteral("edit-copy") ), i18n( "&Clone bias" ), this );
        connect( action, &QAction::triggered, this, &DynamicView::cloneSelected );
        actions.append( action );

        // don't allow deleting a top bias unless it'a an and-bias containing at least one
        // other bias
        QModelIndex parentIndex = index.parent();
        QVariant parentV = model()->data( parentIndex, Dynamic::DynamicModel::PlaylistRole );
        if( !parentV.isValid() || (aBias && aBias->biases().count() > 0) )
        {
            action = new QAction( QIcon::fromTheme( QStringLiteral("edit-delete") ), i18n( "&Delete bias" ), this );
            connect( action, &QAction::triggered, this, &DynamicView::removeSelected );
            actions.append( action );
        }

        if( aBias )
        {
            action = new QAction( QIcon::fromTheme( QStringLiteral("document-new") ), i18n( "&Add new bias" ), this );
            connect( action, &QAction::triggered, this, &DynamicView::addToSelected );
            actions.append( action );
        }
    }

    if( actions.isEmpty() )
        return;

    QMenu menu;
    for( QAction *action : actions )
    {
        if( action )
            menu.addAction( action );
    }

    menu.exec( mapToGlobal( event->pos() ) );
}

