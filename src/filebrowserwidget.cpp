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
#include "playlist/PlaylistModel.h"
#include "TheInstances.h"

#include <QDir>
#include <QDirModel>
#include <QListView>
#include <QVBoxLayout>

#include <KLineEdit>


FileBrowserWidget::FileBrowserWidget( const char *name )
{
    m_edit = new KLineEdit( this );
    connect( m_edit, SIGNAL(returnPressed(QString)),
                   SLOT(setRootDirectory(QString)));
    m_edit->setText( QDir::home().path() );
    setObjectName( name );

    m_model = new QDirModel;
//     m_model->setNameFilters( QStringList() << "mp3" << "ogg" << "flac" );
    m_model->setFilter( QDir::AllDirs | QDir::AllEntries );

    m_view = new QListView( this );
    connect( m_view, SIGNAL(doubleClicked(QModelIndex)),
                     SLOT(setRootDirectory(QModelIndex)));

    m_view->setModel( m_model );

    m_view->setRootIndex(m_model->index(QDir::home().path()));
}


FileBrowserWidget::~FileBrowserWidget()
{
    delete m_model;
}

void
FileBrowserWidget::setRootDirectory( const QString &path )
{
    m_view->setRootIndex(m_model->index(path));
}

void
FileBrowserWidget::setRootDirectory( const QModelIndex &index )
{
    QString url = m_model->filePath( index );
    QFileInfo *fileInfo = new QFileInfo( url );
    if( fileInfo->isDir() )
    {
        m_view->setRootIndex( index );
        m_edit->setText( url );
    }
    else if( fileInfo->isFile() )
    {
        Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( KUrl(url) );
        The::playlistModel()->insertOptioned( track, Playlist::Append );
    }
}

#include "filebrowserwidget.moc"
