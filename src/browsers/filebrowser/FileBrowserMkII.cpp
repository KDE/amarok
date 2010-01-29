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

#include "FileBrowserMkII.h"

#include "Debug.h"
#include "EngineController.h"
#include "FileTreeView.h"
#include "playlist/PlaylistController.h"


#include <KLineEdit>

#include <QDir>
#include <QFileSystemModel>

FileBrowserMkII::FileBrowserMkII( const char * name, QWidget *parent )
    : BrowserCategory( name, parent )
    , m_directoryLoader( 0 )
{

    DEBUG_BLOCK;
    m_searchWidget = new SearchWidget( this, this, false );
    m_searchWidget->setClickMessage( i18n( "Filter Files" ) );

    m_filterTimer.setSingleShot( true );
    connect( &m_filterTimer, SIGNAL( timeout() ), this, SLOT( slotFilterNow() ) );
    
    m_fileSystemModel = new QFileSystemModel( this );
    m_fileSystemModel->setRootPath( QDir::homePath() );
    m_fileSystemModel->setNameFilterDisables( false );
    m_fileSystemModel->setFilter( QDir::AllEntries );

    debug() << "home path: " <<  QDir::homePath();

    m_fileView = new FileTreeView( this );

    debug() << "root index: " << m_fileSystemModel->index( QDir::homePath() ).row();
    

    m_fileView->setModel( m_fileSystemModel );
    m_fileView->setRootIndex( m_fileSystemModel->index( QDir::homePath() ) );

    m_fileView->setDragEnabled( true );
    m_fileView->setSelectionMode( QAbstractItemView::ExtendedSelection );

    connect( m_fileView, SIGNAL( doubleClicked( const QModelIndex & ) ), this, SLOT( itemActivated( const QModelIndex & ) ) );
}


void FileBrowserMkII::itemActivated( const QModelIndex &index )
{
    DEBUG_BLOCK
    KUrl filePath = KUrl( m_fileSystemModel->filePath( index ) );

    debug() << "activated url: " << filePath.url();
    debug() << "filename: " << filePath.fileName();

    if( m_fileSystemModel->isDir( index ) ) {
        debug() << "setting root path to: " << filePath.path();
        m_fileSystemModel->setRootPath( filePath.path() );
        m_fileView->setRootIndex( m_fileSystemModel->index( filePath.path() ) );
    }
    else
    {
        if( EngineController::canDecode( filePath ) )
        {
            QList<KUrl> urls;
            urls << filePath;
            The::playlistController()->insertOptioned( urls, Playlist::AppendAndPlay );
        }
    }
}

void FileBrowserMkII::slotSetFilterTimeout()
{
    KLineEdit *lineEdit = dynamic_cast<KLineEdit*>( sender() );
    if( lineEdit )
    {
        m_currentFilter = lineEdit->text();
        m_filterTimer.stop();
        m_filterTimer.start( 500 );
    }
}

void FileBrowserMkII::slotFilterNow()
{
    m_proxyModel->setFilterFixedString( m_currentFilter );

    QStringList filters;
    filters << m_currentFilter;
    
    m_fileSystemModel->setNameFilters( filters );
}