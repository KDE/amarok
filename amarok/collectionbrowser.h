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
class QStringList;
class QCustomEvent;
class KComboBox;
class KDirLister;


class CollectionBrowser: public QVBox 
{
    Q_OBJECT
    friend class CollectionView;
    
    public:
        CollectionBrowser( const char* name );
    
    private:
    //attributes:
        KComboBox* m_comboBox;
};


class CollectionView : public KListView
{
    Q_OBJECT
    friend class CollectionBrowser;
    
    public:
        CollectionView( CollectionBrowser* parent );
        ~CollectionView();
        
    signals:
        void tagsReady();    
        
    private slots:
        void setupDirs();    
        void scan();        
        void renderView();
        void slotExpanded( QListViewItem* );
        void slotCollapsed( QListViewItem* );    
        
    private:
        void readDir( const KURL& url );
        void dumpDb();
        void customEvent( QCustomEvent* );
        
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
                
/*    class Item : public KIconViewItem
    {
        public:
            Item( QIconView*, const KURL&, const KURL::List&, const uint );
            const KURL& url() const { return m_url; }
        private:
            QString metaString() const;
    
            const KURL m_url;
            int m_numberTracks;
            QString m_length;
            QRect m_bounds;
    };*/
};


#endif /* AMAROK_COLLECTIONBROWSER_H */

#endif /* HAVE_SQLITE */



