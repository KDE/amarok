// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// See COPYING file for licensing information.

#ifndef AMAROK_COLLECTIONBROWSER_H
#define AMAROK_COLLECTIONBROWSER_H

#include "collectiondb.h"

#include <qvbox.h>           //baseclass

#include <klistview.h>       //baseclass
#include <klineedit.h>       //baseclass
#include <qstringlist.h>     //stack allocated
#include <kurl.h>            //stack allocated

class sqlite;
class ThreadWeaver;

class QCString;
class QCustomEvent;
class QDragObject;
class QPixmap;
class QPoint;
class QStringList;

class KDirWatch;
class KPopupMenu;

class CollectionBrowser: public QVBox 
{
    Q_OBJECT
    friend class CollectionView;
    
    public:
        CollectionBrowser( const char* name );

    private slots:
        void slotSetFilter();

    private:
    //attributes:
        enum CatMenuId { IdAlbum, IdArtist, IdGenre, IdYear, IdNone };
        
        KPopupMenu* m_actionsMenu;
        KPopupMenu* m_cat1Menu;
        KPopupMenu* m_cat2Menu;
        KLineEdit* m_searchEdit;
        CollectionView* m_view;
};


class CollectionView : public KListView
{
    Q_OBJECT
    friend class CollectionBrowser;
    
    public:
        class Item : public KListViewItem {
            public:
                Item( QListView* parent )
                    : KListViewItem( parent ) {};
                Item( QListViewItem* parent )
                    : KListViewItem( parent ) {};
                void setUrl( const KURL& url ) { m_url = url; }
                const KURL& url() const { return m_url; }
            private:
                int compare( QListViewItem*, int, bool ) const;
            //attributes:
                KURL m_url;
        };
        friend class Item; // for access to m_category2
    
        CollectionView( CollectionBrowser* parent );
        ~CollectionView();
        
        void setFilter( QString filter ) { m_filter = filter; }
        QString filter() { return m_filter; }
        Item* currentItem() { return static_cast<Item*>( KListView::currentItem() ); }
        
        static CollectionDB* m_insertdb;
        
    signals:
        void tagsReady();    
        
    private slots:
        /** Shows the folder selection widget. */
        void setupDirs();    
        
        void scan();
        void dirDirty( const QString& path );
        
        /** Rebuilds and displays the treeview by querying the database. */
        void renderView();
        
        void slotExpand( QListViewItem* );
        void slotCollapse( QListViewItem* );    
        void cat1Menu( int );
        void cat2Menu( int );
        void rmbPressed( QListViewItem*, const QPoint&, int );
        void makePlaylist();
        void showTrackInfo();
                
    private:
        void customEvent( QCustomEvent* );
        void startDrag();
        KURL::List listSelected();
               
        QString catForId( int id ) const;
        int idForCat( const QString& cat ) const;
        QPixmap iconForCat( const QString& cat ) const;
        
    //attributes:
        //bump DATABASE_VERSION whenever changes to the table structure are made. will remove old db file.
        static const int DATABASE_VERSION = 5;
        static CollectionDB* m_db;
        QCString m_databasePath;
        
        CollectionBrowser* m_parent;
        ThreadWeaver* m_weaver;
        KDirWatch* m_dirWatch;
        QString m_filter;
        QStringList m_dirs;
        QString m_category1;
        QString m_category2;
        bool m_recursively;
        bool m_monitor;
};


#endif /* AMAROK_COLLECTIONBROWSER_H */
