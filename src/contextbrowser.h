// (c) Christian Muehlhaeuser 2004
// See COPYING file for licensing information


#ifndef AMAROK_CONTEXTBROWSER_H
#define AMAROK_CONTEXTBROWSER_H

#include "engineobserver.h"
#include <qvbox.h>
#include <klistview.h>
#include <kparts/browserextension.h>
#include <kurl.h>

class KHTMLPart;
class QPalette;
class MetaBundle;
class CollectionDB;

class ContextBrowser : public QVBox, public EngineObserver
{
    Q_OBJECT

    public:
        ContextBrowser( const char *name );
        ~ContextBrowser();

    public slots:
        void openURLRequest(const KURL &url, const KParts::URLArgs & );
        
    protected:
        void engineNewMetaData( const MetaBundle&, bool );
        void paletteChange( const QPalette& );

    private:
        void render();
        void setStyleSheet();
        void showIntroduction();
        void showHome();
        void showCurrentTrack();
        
        KHTMLPart *browser;
        MetaBundle *m_currentTrack;
        CollectionDB *m_db;
        QString m_styleSheet;
        KURL m_url;
};

#endif /* AMAROK_CONTEXTBROWSER_H */
