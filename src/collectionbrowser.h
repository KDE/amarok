// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// See COPYING file for licensing information.

#ifndef AMAROK_COLLECTIONBROWSER_H
#define AMAROK_COLLECTIONBROWSER_H

#include "qvbox.h"           //baseclass

#include <klistview.h>       //baseclass
#include <qstringlist.h>     //stack allocated
#include <kurl.h>            //stack allocated

#include "collectiondb.h"

class ClickLineEdit;
class CollectionDB;
class sqlite;

class QCString;
class QDragObject;
class QPixmap;
class QPoint;
class QStringList;

class KAction;
class KPopupMenu;
class KProgress;

class CollectionBrowser: public QVBox
{
    Q_OBJECT
    friend class CollectionView;

    public:
        CollectionBrowser( const char* name );

    public slots:
        void setupDirs();

    private slots:
        void slotSetFilterTimeout();
        void slotSetFilter();

    private:
        //attributes:
        enum CatMenuId { IdAlbum = 1, IdArtist = 2, IdGenre = 4, IdYear = 8 , IdScan = 16, IdNone = 32,
                         IdArtistAlbum = 64, IdGenreArtist = 128, IdGenreArtistAlbum = 256 };

        KAction* m_configureAction;
        KAction* m_scanAction;
        KAction* m_treeViewAction;
        KAction* m_flatViewAction;

        KPopupMenu* m_categoryMenu;
        KPopupMenu* m_cat1Menu;
        KPopupMenu* m_cat2Menu;
        KPopupMenu* m_cat3Menu;
        KLineEdit* m_searchEdit;
        CollectionView* m_view;
        QTimer* m_timer;
};


class CollectionView : public KListView
{
    Q_OBJECT
    friend class CollectionBrowser;

    public:
        enum ViewMode  { modeTreeView, modeFlatView };

        class Item : public KListViewItem {
            public:
                Item( QListView* parent )
                    : KListViewItem( parent ) {};
                Item( QListViewItem* parent )
                    : KListViewItem( parent ) {};
                void setUrl( const QString& url ) { m_url.setPath( url ); }
                const KURL& url() const { return m_url; }

            //attributes:
                KURL m_url;
        };
        friend class Item; // for access to m_cat2

        CollectionView( CollectionBrowser* parent );
        ~CollectionView();

        static CollectionView* instance() { return m_instance; }
        /** Rebuilds and displays the treeview by querying the database. */
        void renderView();
        void setFilter( const QString &filter ) { m_filter = filter; }
        QString filter() { return m_filter; }
        Item* currentItem() { return static_cast<Item*>( KListView::currentItem() ); }

    public slots:
        void setTreeMode() { setViewMode( modeTreeView ); };
        void setFlatMode() { setViewMode( modeFlatView ); };

        void presetMenu( int id );
        void cat1Menu( int id, bool rerender = true );
        void cat2Menu( int id, bool rerender = true );
        void cat3Menu( int id, bool rerender = true );

    private slots:
        void setupDirs();

        void scanStarted();
        void scanDone( bool changed = true );

        void cacheItem( QListViewItem* item );
        void slotExpand( QListViewItem* );
        void slotCollapse( QListViewItem* );
        void enableCat3Menu( bool );
        void invokeItem( QListViewItem* );
        void rmbPressed( QListViewItem*, const QPoint&, int );

        /** Tries to download the cover image from Amazon.com */
        void fetchCover();
        /** Shows dialog with information on selected track */
        void showTrackInfo();

    private:
        void setViewMode( int mode, bool rerender = true );
        void startDrag();
        KURL::List listSelected();

        QPixmap iconForCategory( const int cat ) const;
        QString captionForCategory( const int cat ) const;

        void viewportPaintEvent( QPaintEvent* ); // Reimplemented from KListView

    //attributes:
        static CollectionView* m_instance;

        CollectionBrowser* m_parent;
        QString m_filter;
        int m_cat1;
        int m_cat2;
        int m_cat3;
        int m_viewMode;

        QStringList m_cacheItem;
};


#endif /* AMAROK_COLLECTIONBROWSER_H */
