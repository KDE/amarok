// (c) Christian Muehlhaeuser 2004
// See COPYING file for licensing information


#ifndef AMAROK_CONTEXTBROWSER_H
#define AMAROK_CONTEXTBROWSER_H

#include <qvbox.h>
#include <klistview.h>
#include <kparts/browserextension.h>

class KURL;
class KHTMLPart;
class MetaBundle;
class CollectionDB;

class ContextBrowser : public QVBox
{
    Q_OBJECT

    public:
        ContextBrowser( const char *name );
        ~ContextBrowser();
        
        void showContextForItem( const MetaBundle &bundle );
        void sqlInit();

    public slots:
        void openURLRequest(const KURL &url, const KParts::URLArgs & );

    private:
        KHTMLPart *browser;
        CollectionDB *m_db;
};

#endif /* AMAROK_CONTEXTBROWSER_H */
