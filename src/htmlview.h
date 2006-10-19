// (c) 2005 Christian Muehlhaeuser <chris@chris.de>
// (c) 2006 Seb Ruiz <me@sebruiz.net>
// License: GNU General Public License V2


#ifndef AMAROK_HTMLVIEW_H
#define AMAROK_HTMLVIEW_H

#include <khtml_events.h>
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
        // for overloading the drag and drop image
        void   khtmlMouseMoveEvent( khtml::MouseMoveEvent *event );
        void   khtmlMousePressEvent( khtml::MousePressEvent *event );
        void   khtmlMouseReleaseEvent( khtml::MouseReleaseEvent *event );

        static KTempFile *m_bgGradientImage;
        static KTempFile *m_headerGradientImage;
        static KTempFile *m_shadowGradientImage;
        static int m_instances;
};

#endif /* AMAROK_HTMLVIEW_H */
