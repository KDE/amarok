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
        /**
         * Shows the folder selection widget
         */
        void setupDirs();    
        
        void scan();        
        
        /**
         * Rebuilds and displays the treeview by querying the database
         */
        void renderView();
        
        void slotExpanded( QListViewItem* );
        void slotCollapsed( QListViewItem* );    
        void actionsMenu( int );
        
    private:
        void dumpDb();
        void customEvent( QCustomEvent* );
        void startDrag();
        
        /**
         * Executes an SQL statement on the already opened database
         * @param statement SQL program to execute. Only one SQL statement is allowed.
         * @retval values   will contain the queried data, set to NULL if not used
         * @retval names    will contain all column names, set to NULL if not used
         * @return          true if successful
         */
        bool execSql( const QCString& statement, QStringList* const values = 0, QStringList* const names = 0 );
            
    //attributes:
        CollectionBrowser* m_parent;
        ThreadWeaver* m_weaver;
        sqlite* m_db;                
        QStringList m_dirs;
        QString m_category;
        bool m_recursively;
        bool m_monitor;
};


#endif /* AMAROK_COLLECTIONBROWSER_H */

#endif /* HAVE_SQLITE */



