// (c) Max Howell 2004
// See COPYING file for licensing information

#ifndef PLAYLISTBROWSER_H
#define PLAYLISTBROWSER_H

#include <kiconview.h>
#include <kurl.h> //KURL::List

class QColorGroup;
class QCustomEvent;
class QDragObject;
class QPainter;
class QPixmap;

class PlaylistBrowser : public KIconView
{
Q_OBJECT

public:
    PlaylistBrowser( const char* );
    ~PlaylistBrowser();

    QSize sizeHint() const { return minimumSize(); }
    QDragObject* dragObject();

    static QPixmap findCoverArt( const KURL& );

private:
    void customEvent( QCustomEvent* );

    class Item : public KIconViewItem
    {
    public:
        Item( QIconView*, const KURL&, const KURL::List&, const uint );
        const KURL& url() const { return m_url; }
    private:
        void paintItem( QPainter*, const QColorGroup& );
        void calcRect( const QString& = QString::null );
        QString metaString() const;

        const KURL m_url;
        int m_numberTracks;
        QString m_length;
    };

    Item *currentItem() const { return (Item *)KIconView::currentItem(); }
};


inline QString
fileBasename( const QString &fileName )
{
    return fileName.mid( 0, fileName.findRev( '.' ) );
}

inline QString
fileExtension( const QString &fileName )
{
    return fileName.mid( fileName.findRev( '.' ) + 1 );
}

#endif
