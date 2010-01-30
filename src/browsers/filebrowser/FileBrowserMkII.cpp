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
#include "FileView.h"
#include "MimeTypeFilterProxyModel.h"
#include "playlist/PlaylistController.h"


#include <KLineEdit>
#include <KDirModel>
#include <KDirLister>
#include <QDir>

FileBrowserMkII::FileBrowserMkII( const char * name, QWidget *parent )
    : BrowserCategory( name, parent )
    , m_directoryLoader( 0 )
{

    DEBUG_BLOCK;
    m_searchWidget = new SearchWidget( this, this, false );
    m_searchWidget->setClickMessage( i18n( "Filter Files" ) );

    m_filterTimer.setSingleShot( true );
    connect( &m_filterTimer, SIGNAL( timeout() ), this, SLOT( slotFilterNow() ) );

    m_kdirModel = new KDirModel( this );
    m_kdirModel->dirLister()->openUrl( KUrl( QDir::homePath() ) );

    m_mimeFilterProxyModel = new MimeTypeFilterProxyModel( EngineController::supportedMimeTypes(), this );
    m_mimeFilterProxyModel->setSourceModel( m_kdirModel );

    m_proxyModel = new QSortFilterProxyModel( this );
    m_proxyModel->setSourceModel( m_mimeFilterProxyModel );

    debug() << "home path: " <<  QDir::homePath();

    m_fileView = new FileView( this );
    m_fileView->setModel( m_proxyModel );

    m_fileView->setDragEnabled( true );
    m_fileView->setSelectionMode( QAbstractItemView::ExtendedSelection );

    connect( m_fileView, SIGNAL( activated( const QModelIndex & ) ), this, SLOT( itemActivated( const QModelIndex & ) ) );
}


void FileBrowserMkII::itemActivated( const QModelIndex &index )
{
    DEBUG_BLOCK
    KFileItem file = index.data( KDirModel::FileItemRole ).value<KFileItem>();
    KUrl filePath = file.url();

    debug() << "activated url: " << filePath.url();
    debug() << "filename: " << filePath.fileName();

    if( file.isDir() ) {
        debug() << "setting root path to: " << filePath.path();
        m_kdirModel->dirLister()->openUrl( filePath );
        m_fileView->setRootIndex( index );
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
}