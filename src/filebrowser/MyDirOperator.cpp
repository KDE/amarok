/*
    Copyright (c) 2008 Dan Meltzer <hydrogen@notyetimplemented.com>
              (c) 2008 Seb Ruiz <ruiz@kde.org>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "MyDirOperator.h"

#include "Debug.h"
#include "MainWindow.h"
#include "collection/Collection.h"
#include "collection/CollectionManager.h"
#include "playlist/PlaylistModel.h"

#include <KActionCollection>

#include <QAbstractItemView>
#include <QMenu>

/**
 * Stores a collection associated with an action for move/copy to collection
 */
class CollectionAction : public QAction
{
    public:
        CollectionAction( Collection *coll, QObject *parent = 0 )
            : QAction( parent )
            , m_collection( coll )
        {
           setText( m_collection->prettyName() );
        }

        Collection *collection() const { return m_collection; }

    private:
        Collection *m_collection;
};


MyDirOperator::MyDirOperator( const KUrl &url, QWidget *parent )
    : KDirOperator( url, parent )
{
    MyDirLister* dirlister = new MyDirLister( true );
    dirlister->setMainWindow( The::mainWindow() );

    setDirLister( dirlister );
    setView( KFile::Simple );

    view()->setSelectionMode(QAbstractItemView::ExtendedSelection);
    view()->setContentsMargins(0,0,0,0);
    view()->setFrameShape( QFrame::NoFrame );

    connect( this, SIGNAL( fileSelected( const KFileItem& ) ),
             this,   SLOT( fileSelected( const KFileItem& ) ) );
    
    //WARNING: This signal is only available under KDE4.2 libraries, so the functionality
    //won't exist for any users on KDE4.1.
    connect( this, SIGNAL( contextMenuAboutToShow( const KFileItem &item, QMenu *menu ) ),
             this,   SLOT( contextMenuAboutToShow( const KFileItem &item, QMenu *menu ) ) );
}

MyDirOperator::~MyDirOperator()
{
    DEBUG_BLOCK
}

void MyDirOperator::fileSelected( const KFileItem & /*file*/ )
{
  const KFileItemList list = selectedItems();

  KUrl::List urlList;
  foreach( const KFileItem& item, list )
  {
      urlList << item.url();
  }

  Meta::TrackList trackList = CollectionManager::instance()->tracksForUrls( urlList );
  The::playlistModel()->insertOptioned( trackList, Playlist::AppendAndPlay );
  view()->selectionModel()->clear();
}

void MyDirOperator::contextMenuAboutToShow( const KFileItem &item, QMenu *menu )
{
    DEBUG_BLOCK
    Q_UNUSED( item );
    const KFileItemList list = selectedItems();

    KUrl::List urlList;
    foreach( const KFileItem &item, list )
    {
        urlList << item.url();
    }

    Meta::TrackList trackList = CollectionManager::instance()->tracksForUrls( urlList );

    QList<Collection*> writableCollections;
    QHash<Collection*, CollectionManager::CollectionStatus> hash = CollectionManager::instance()->collections();
    QHash<Collection*, CollectionManager::CollectionStatus>::const_iterator it = hash.constBegin();
    while( it != hash.constEnd() )
    {
        Collection *coll = it.key();
        if( coll && coll->isWritable() )
        {
            debug() << "got writable collection";
            writableCollections.append( coll );
        }
        ++it;
    }
    if( !writableCollections.isEmpty() )
    {
        QMenu *copyMenu = new QMenu( i18n( "Move to Collection" ), menu );
        foreach( Collection *coll, writableCollections )
        {
            CollectionAction *moveAction = new CollectionAction( coll, this );
            connect( moveAction, SIGNAL( triggered() ), this, SLOT( slotMoveTracks() ) );
            copyMenu->addAction( moveAction );
        }
        menu->addMenu( copyMenu );
    }
}


#include "MyDirOperator.moc"

