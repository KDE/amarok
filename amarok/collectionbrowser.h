// (c) Mark Kretschmann 2004
// See COPYING file for licensing information


#ifndef AMAROK_COLLECTIONBROWSER_H
#define AMAROK_COLLECTIONBROWSER_H

#include <kiconview.h>

class QSqlDatabase;


class CollectionBrowser : public KIconView
{
    Q_OBJECT
    
    public:
        CollectionBrowser( const char* name );
        ~CollectionBrowser();
        
    private:

    //attributes:
        QSqlDatabase* m_pDb;
        
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

