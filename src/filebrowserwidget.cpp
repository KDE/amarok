/***************************************************************************
 *   Copyright (c) 2007  Dan Meltzer <hydrogen@notyetimplemented.com>      *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.         *
 ***************************************************************************/


#include "collection/CollectionManager.h"
#include "filebrowserwidget.h"
#include "meta/meta.h"
#include "mydirlister.h"
#include "playlist/PlaylistModel.h"
#include "TheInstances.h"

#include <QDir>
#include <QListView>
#include <QVBoxLayout>

#include <KActionCollection>
#include <KDirModel>
#include <kdirsortfilterproxymodel.h>
#include <KUrlComboBox>
#include <KUrlCompletion>


FileBrowserWidget::FileBrowserWidget( const char *name )
{
    m_combo = new KUrlComboBox( KUrlComboBox::Directories, true, this );

    m_combo->setCompletionObject( new KUrlCompletion( KUrlCompletion::DirCompletion ) );
    m_combo->setAutoDeleteCompletionObject( true );
    m_combo->setMaxItems( 9 );

    connect( m_combo, SIGNAL(returnPressed(QString)),
                   SLOT(setRootDirectory(QString)));
    m_combo->setUrl( KUrl(QDir::home().path()) );
    setObjectName( name );

    m_model = new KDirModel( this );
    m_model->setDirLister( new MyDirLister( true ) );
    m_model->dirLister()->setShowingDotFiles( false );

    m_sortModel = new KDirSortFilterProxyModel( this );
    m_sortModel->setSourceModel( m_model );

    m_view = new QListView( this );

    m_view->setModel( m_sortModel );

    connect( m_view, SIGNAL(doubleClicked(QModelIndex)),
             SLOT(setRootDirectory(QModelIndex)));

    m_model->dirLister()->openUrl( KUrl( "~" ) );

    setFocusProxy( m_view );
}


FileBrowserWidget::~FileBrowserWidget()
{
    delete m_model;
}

void
FileBrowserWidget::setRootDirectory( const QString &path )
{
    m_model->dirLister()->openUrl( KUrl( path ) );
}

void
FileBrowserWidget::setRootDirectory( const QModelIndex &index )
{
    QModelIndex sourceIndex = m_sortModel->mapToSource( index );
    KFileItem fi = m_model->itemForIndex( sourceIndex );

    if( fi.isDir() )
    {
        m_model->dirLister()->openUrl( fi.url() );
        m_combo->setUrl( fi.url() );
    }
    else
    {
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( fi.url() );
        The::playlistModel()->insertOptioned( track, Playlist::Append );
    }
}

#include "filebrowserwidget.moc"
