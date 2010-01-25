/****************************************************************************************
 * Copyright (c) 2010 Nikolaj Hald Nielsen <nhn@kde.org>                                *
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

#include "FileTreeView.h"

#include "Debug.h"
#include "EngineController.h"
#include "playlist/PlaylistController.h"

#include <KIcon>
#include <KLocale>
#include <KMenu>
#include <KUrl>

#include <QContextMenuEvent>
#include <QFileSystemModel>

FileTreeView::FileTreeView( QWidget * parent )
    : Amarok::PrettyTreeView( parent )
    , m_appendAction( 0 )
    , m_loadAction( 0 )
{
}

void FileTreeView::contextMenuEvent ( QContextMenuEvent * e )
{

    if( !model() )
        return;

    QModelIndexList indices = selectedIndexes();

    // Abort if nothing is selected
    if( indices.isEmpty() )
        return;

    

    KMenu* menu = new KMenu( this );

    QList<QAction *> actions = actionsForIndices( indices );

    foreach( QAction * action, actions )
        menu->addAction( action );
    
    menu->exec( e->globalPos() );
 
}

void FileTreeView::slotAppendToPlaylist()
{
    addSelectionToPlaylist( false );
}


void FileTreeView::slotReplacePlaylist()
{
    addSelectionToPlaylist( true );
}

QList<QAction *> FileTreeView::actionsForIndices( const QModelIndexList &indices )
{

    QList<QAction *> actions;
    
    if( !indices.isEmpty() )
    {
        if( m_appendAction == 0 )
        {
            m_appendAction = new QAction( KIcon( "media-track-add-amarok" ), i18n( "&Add to Playlist" ), this );
            m_appendAction->setProperty( "popupdropper_svg_id", "append" );
            connect( m_appendAction, SIGNAL( triggered() ), this, SLOT( slotAppendToPlaylist() ) );
        }

        actions.append( m_appendAction );

        if( m_loadAction == 0 )
        {
            m_loadAction = new QAction( i18nc( "Replace the currently loaded tracks with these", "&Replace Playlist" ), this );
            m_loadAction->setProperty( "popupdropper_svg_id", "load" );
            connect( m_loadAction, SIGNAL( triggered() ), this, SLOT( slotReplacePlaylist() ) );
        }

        actions.append( m_loadAction );
    }

    return actions;
}

void FileTreeView::addSelectionToPlaylist( bool replace )
{
    DEBUG_BLOCK
    QModelIndexList indices = selectedIndexes();

    if( indices.count() == 0 )
        return;
    
    QFileSystemModel * fsModel = qobject_cast<QFileSystemModel *>( model() );
    
    if( fsModel )
    {
        QList<KUrl> urls;
        
        foreach( QModelIndex index, indices )
        {
            QString path = fsModel->filePath( index );
            debug() << "file path: " << path;
            if( EngineController::canDecode( path ) || fsModel->isDir( index ) )
            {
                urls << KUrl( path );
            }
        }

        The::playlistController()->insertOptioned( urls, replace ? Playlist::Replace : Playlist::AppendAndPlay );
    }
}

#include "FileTreeView.moc"