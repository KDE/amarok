// (c) Mark Kretschmann 2004
// See COPYING file for licensing information


#include <config.h>
#ifdef HAVE_SQLITE

#ifndef AMAROK_COLLECTIONBROWSER_H
#define AMAROK_COLLECTIONBROWSER_H

#include <kiconview.h>
#include <kurl.h>

class ThreadWeaver;
class QCustomEvent;


class CollectionBrowser : public KIconView
{
    Q_OBJECT
    
    public:
        CollectionBrowser( const char* name );
        ~CollectionBrowser();
        
    private:
        void customEvent( QCustomEvent* );
    
    //attributes:
        KURL::List m_dirs;
        ThreadWeaver* m_weaver;
                
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

