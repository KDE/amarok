// (c) Max Howell 2004
// See COPYING file for licensing information

#ifndef PLAYLISTBROWSER_H
#define PLAYLISTBROWSER_H

#include <kiconview.h>
#include <kurl.h> //KURL::List

class QDragObject;
class QEvent;

class PlaylistBrowser : public KIconView
{
Q_OBJECT

public:
    PlaylistBrowser( const char* );
    ~PlaylistBrowser();

    void  newPlaylist( const KURL::List& );
    QSize sizeHint() const { return minimumSize(); }
    void  customEvent( QEvent* );
    QDragObject* dragObject();

    class Item : public KIconViewItem
    {
    public:
        Item( QIconView*, const KURL& );
        const KURL& url() const { return m_url; }
    private:
        const KURL m_url;
    };

    Item *currentItem() const { return (Item *)KIconView::currentItem(); }
};

#endif
