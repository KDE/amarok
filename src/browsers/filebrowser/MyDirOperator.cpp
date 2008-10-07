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

#include "MainWindow.h"
#include "collection/Collection.h"
#include "collection/CollectionLocation.h"
#include "collection/CollectionManager.h"
#include "playlist/PlaylistController.h"

#include <KActionCollection>
#include <KActionMenu>
#include <KLocale>

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
    
    //FIXME: This signal is only available under KDE4.2 libraries, so remove the ActionCollection hack
    //when we bump kdelibs dep.
    //connect( this, SIGNAL( contextMenuAboutToShow( const KFileItem &item, QMenu *menu ) ),
    //         this,   SLOT( contextMenuAboutToShow( const KFileItem &item, QMenu *menu ) ) );

    //HACK: crafty method to hijack the context menu
    KActionMenu *actionMenu = static_cast<KActionMenu*>( actionCollection()->action( "popupMenu" ) );
    if( actionMenu )
    {
        KMenu *menu = actionMenu->menu();
        connect( menu, SIGNAL( aboutToShowContextMenu( KMenu *menu, QAction *menuAction, QMenu *ctxMenu ) ),
                 this,   SLOT( aboutToShowContextMenu( KMenu *menu, QAction *menuAction, QMenu *ctxMenu ) ) );

        connect( menu, SIGNAL( aboutToShow() ), this, SLOT( aboutToShowContextMenu() ) );
    }
}

MyDirOperator::~MyDirOperator()
{
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
    The::playlistController()->insertOptioned( trackList, Playlist::AppendAndPlay );
    view()->selectionModel()->clear();
}

void MyDirOperator::aboutToShowContextMenu()
{
    QMenu *menu = dynamic_cast<QMenu*>( sender() );
    if( !menu )
        return;

    const KFileItemList list = selectedItems();

    if( list.isEmpty() ) // or not file?
        return;

    KUrl::List urlList;
    foreach( const KFileItem &item, list )
    {
        urlList << item.url();
    }

    QList<Collection*> writableCollections;
    QHash<Collection*, CollectionManager::CollectionStatus> hash = CollectionManager::instance()->collections();
    QHash<Collection*, CollectionManager::CollectionStatus>::const_iterator it = hash.constBegin();
    while( it != hash.constEnd() )
    {
        Collection *coll = it.key();
        if( coll && coll->isWritable() )
        {
            writableCollections.append( coll );
        }
        ++it;
    }
    if( !writableCollections.isEmpty() )
    {
        QMenu *copyMenu = new QMenu( i18n( "Move to Collection" ), this );
        foreach( Collection *coll, writableCollections )
        {
            CollectionAction *moveAction = new CollectionAction( coll, this );
            connect( moveAction, SIGNAL( triggered() ), this, SLOT( slotMoveTracks() ) );
            copyMenu->addAction( moveAction );
        }
        menu->addMenu( copyMenu );
    }
}

void
MyDirOperator::slotMoveTracks()
{
    CollectionAction *action = dynamic_cast<CollectionAction*>( sender() );
    if( !action )
        return;

    const KFileItemList list = selectedItems();
    if( list.isEmpty() )
        return;

    CollectionLocation *source      = new CollectionLocation();
    CollectionLocation *destination = action->collection()->location();

    CollectionManager *cm = CollectionManager::instance();
    Meta::TrackList tracks = cm->tracksForUrls( list.urlList() );

    source->prepareMove( tracks, destination );
}


#include "MyDirOperator.moc"

