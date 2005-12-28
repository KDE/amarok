// (c) 2005 Christian Muehlhaeuser <chris@chris.de>
// License: GNU General Public License V2


#ifndef AMAROK_HTMLVIEW_H
#define AMAROK_HTMLVIEW_H

#include <khtml_part.h>
#include <khtmlview.h>

class KTempFile;

class HTMLView : public KHTMLPart
{
    Q_OBJECT

    public:
        HTMLView( QWidget *parentWidget = 0, const char *widgetname = 0, const bool DNDEnabled = false, const bool JScriptEnabled = true );
       ~HTMLView();

        static QString loadStyleSheet();
        static void    openURLRequest(const KURL &url );
        void   set( const QString& data );
        static void paletteChange();

    private:
        static KTempFile *m_bgGradientImage;
        static KTempFile *m_headerGradientImage;
        static KTempFile *m_shadowGradientImage;
        static int m_instances;
};

#endif /* AMAROK_HTMLVIEW_H */
