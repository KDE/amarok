// (c) Mark Kretschmann 2004
// See COPYING file for licensing information


#include "config.h"

#ifdef HAVE_SQLITE

#ifndef AMAROK_COLLECTIONBROWSER_H
#define AMAROK_COLLECTIONBROWSER_H

#include <qvbox.h>           //baseclass

#include <klistview.h>       //baseclass
#include <qstringlist.h>     //stack allocated
#include <kurl.h>            //stack allocated

class sqlite;
class ThreadWeaver;

class QCString;
class QDragObject;
class QStringList;
class QCustomEvent;
class KDirLister;
class KPopupMenu;

class CollectionBrowser: public QVBox 
{
    Q_OBJECT
    friend class CollectionView;
    
    public:
        CollectionBrowser( const char* name );
    
    private:
    //attributes:
        enum actionsMenuIds { IdAlbum, IdArtist, IdGenre, IdYear };
        
        KPopupMenu* m_actionsMenu;
        KPopupMenu* m_catMenu;
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
            //attributes:
                KURL m_url;
        };
    
        CollectionView( CollectionBrowser* parent );
        ~CollectionView();
        
        Item* currentItem() { return static_cast<Item*>( KListView::currentItem() ); }
        
    signals:
        void tagsReady();    
        
    private slots:
        void setupDirs();    
        void scan();        
        void renderView();
        void slotExpanded( QListViewItem* );
        void slotCollapsed( QListViewItem* );    
        void actionsMenu( int );
        
    private:
        void readDir( const KURL& url );
        void dumpDb();
        void customEvent( QCustomEvent* );
        void startDrag();
        
        /**
        * Executes an SQL statement on the already opened database
        * @param statement SQL program to execute. Only one SQL statement is allowed.
        * @out values      will contain the queried data, set to NULL if not used
        * @out names       will contain all column names, set to NULL if not used
        * @return          true if successful
        */
        bool execSql( const QCString& statement, QStringList* const values, QStringList* const names );
            
    //attributes:
        CollectionBrowser* m_parent;
        ThreadWeaver* m_weaver;
        KDirLister* m_dirLister;
        sqlite* m_db;                
        QStringList m_dirs;
        QString m_category;
};


#endif /* AMAROK_COLLECTIONBROWSER_H */

#endif /* HAVE_SQLITE */



