// (c) 2005 Christian Muehlhaeuser <chris@chris.de>
// (c) 2006 Seb Ruiz <me@sebruiz.net>
// License: GNU General Public License V2


#ifndef AMAROK_HTMLVIEW_H
#define AMAROK_HTMLVIEW_H

#include <khtml_events.h>
#include <khtml_part.h>
#include <khtmlview.h>

class KAction;
class KTemporaryFile;

class HTMLView : public KHTMLPart
{
    Q_OBJECT

    public:
        explicit HTMLView( QWidget *parentWidget = 0, const char *widgetname = 0, const bool DNDEnabled = false, const bool JScriptEnabled = true );
       ~HTMLView();

        static QString loadStyleSheet();
        static void    openUrlRequest(const KUrl &url );
        void   set( const QString& data );
        static void paletteChange();

    private:
        static KTemporaryFile *m_bgGradientImage;
        static KTemporaryFile *m_headerGradientImage;
        static KTemporaryFile *m_shadowGradientImage;
        static int m_instances;

        KAction *m_selectAll;
        KAction *m_copy;

    private slots:
        void enableCopyAction();
        void selectAll();
        void copyText();
};

#endif /* AMAROK_HTMLVIEW_H */
