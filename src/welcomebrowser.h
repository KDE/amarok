// (c) Christian Muehlhaeuser 2004
// See COPYING file for licensing information


#ifndef AMAROK_WELCOMEBROWSER_H
#define AMAROK_WELCOMEBROWSER_H

#include <kparts/browserextension.h>
#include <kurl.h>

#include <qvbox.h>

class KHTMLPart;
class QPalette;

class WelcomeBrowser : public QVBox
{
    Q_OBJECT

    public:
        WelcomeBrowser(  QObject* parent, const char *name );
        ~WelcomeBrowser();

    protected:
        void paletteChange( const QPalette& );

    private:
        void setStyleSheet();
        void showPage();
        
        KHTMLPart *browser;
        QString m_styleSheet;
};

#endif /* AMAROK_WELCOMEBROWSER_H */
