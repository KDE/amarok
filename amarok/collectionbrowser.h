// (c) Mark Kretschmann 2004
// See COPYING file for licensing information


#include "config.h"

#ifdef HAVE_SQLITE

#ifndef AMAROK_COLLECTIONBROWSER_H
#define AMAROK_COLLECTIONBROWSER_H

#include <qvbox.h>    //baseclass

#include <kiconview.h>
#include <kurl.h>

class sqlite;
class ThreadWeaver;

class QCString;
class QStringList;
class QCustomEvent;
class KDirLister;

class CollectionBrowser: public QVBox 
{
    Q_OBJECT

    friend class Browser;
    
    public:
        CollectionBrowser( const char* name );
    
    private slots:
        void setupDirs();    
    
    private:
    //attributes:
        KURL::List m_dirs;
        
        class Browser : public KIconView
        {
            friend class CollectionBrowser;
            
            public:
                Browser( QWidget* parent );
                ~Browser();
                
                
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
        }; /* class Browser */

}; /* class CollectionBrowser */

#endif /* AMAROK_COLLECTIONBROWSER_H */

#endif /* HAVE_SQLITE */



