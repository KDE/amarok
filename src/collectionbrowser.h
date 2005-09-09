// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2005 Gábor Lehel <illissius@gmail.com>
// See COPYING file for licensing information.

#ifndef AMAROK_COLLECTIONBROWSER_H
#define AMAROK_COLLECTIONBROWSER_H

#include "qlabel.h"
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
class KRadioAction;
class KPopupMenu;
class KProgress;

class CollectionView;
class CollectionItem;

class CollectionBrowser: public QVBox
{
    Q_OBJECT
    friend class CollectionView;

    public:
        CollectionBrowser( const char* name );
        void refreshInfo();
        virtual bool eventFilter( QObject*, QEvent* );

    public slots:
        void setupDirs();

    private slots:
        void slotSetFilterTimeout();
        void slotSetFilter();

    private:
        //attributes:
        enum CatMenuId { IdAlbum = 1, IdArtist = 2, IdGenre = 4, IdYear = 8 , IdScan = 16, IdNone = 32,
                    IdArtistAlbum = 64, IdGenreArtist = 128, IdGenreArtistAlbum = 256, IdVisYearAlbum = 512, IdArtistVisYearAlbum = 1024 };

        KAction* m_configureAction;
        KAction* m_scanAction;
        KRadioAction* m_treeViewAction;
        KRadioAction* m_flatViewAction;

        KPopupMenu* m_categoryMenu;
        KPopupMenu* m_cat1Menu;
        KPopupMenu* m_cat2Menu;
        KPopupMenu* m_cat3Menu;
        KLineEdit*  m_searchEdit;
        CollectionView* m_view;
        QLabel*     m_infoA;
        QLabel*     m_infoB;
        QTimer*     m_timer;

    friend class CollectionItem; // for CatMenuId
};


class CollectionItem : public KListViewItem {
    public:
        CollectionItem( QListView* parent, int cat )
            : KListViewItem( parent )
            , m_cat( cat ) {};
        CollectionItem( QListView* parent )
            : KListViewItem( parent ) {};
        CollectionItem( QListViewItem* parent, int cat )
            : KListViewItem( parent )
            , m_cat( cat ) {};
        CollectionItem( QListViewItem* parent )
            : KListViewItem( parent ) {};
        void setUrl( const QString& url ) { m_url.setPath( url ); }
        const KURL& url() const { return m_url; }

        virtual void sortChildItems ( int column, bool ascending ); //reimplemented

    private:
        //for sorting
        virtual int compare( QListViewItem*, int, bool ) const; //reimplemented

    //attributes:
        KURL m_url;
        int m_cat;
};


class CollectionView : public KListView
{
    Q_OBJECT
    friend class CollectionBrowser;

    public:
        enum ViewMode  { modeTreeView, modeFlatView };

        friend class CollectionItem; // for access to m_cat2
        friend class ContextBrowser; // for setupDirs()

        CollectionView( CollectionBrowser* parent );
        ~CollectionView();

        static CollectionView* instance() { return m_instance; }
        void setFilter( const QString &filter ) { m_filter = filter; }
        QString filter() { return m_filter; }
        CollectionItem* currentItem() { return static_cast<CollectionItem*>( KListView::currentItem() ); }

        int trackDepth() { return m_trackDepth; }

    public slots:
        /** Rebuilds and displays the treeview by querying the database. */
        void renderView();

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

        void slotExpand( QListViewItem* );
        void slotCollapse( QListViewItem* );
        void enableCat3Menu( bool );
        void invokeItem( QListViewItem* );
        void rmbPressed( QListViewItem*, const QPoint&, int );
        void selectAll() {QListView::selectAll(true); }
        /** Tries to download the cover image from Amazon.com */
        void fetchCover();
        /** Shows dialog with information on selected track */
        void showTrackInfo();

    private:
        void setViewMode( int mode, bool rerender = true );
        void startDrag();
        KURL::List listSelected();

        void playlistFromURLs( const KURL::List &urls );

        QPixmap iconForCategory( const int cat ) const;
        QString captionForCategory( const int cat ) const;

        /** Rebuild selections, viewport and expanded items after reloads */
        void cacheView();
        void restoreView();

        // avoid duplicated code
        inline void manipulateThe( QString &original, bool reverse = false );
        inline bool endsInThe( const QString & text );
        inline void yearAlbumCalc( QString &year, QString &text );
        inline void updateTrackDepth();

        /**Call when a category has changed **/
        void updateColumnHeader();
        // Reimplemented from KListView
        void viewportPaintEvent( QPaintEvent* );
        void viewportResizeEvent( QResizeEvent* );

    //attributes:
        static CollectionView* m_instance;

        CollectionBrowser* m_parent;
        QString m_filter;
        int m_cat1;
        int m_cat2;
        int m_cat3;
        int m_trackDepth;
        int m_viewMode;

        QValueList<QStringList> m_cacheOpenItemPaths;
        QString                 m_cacheViewportTopItem;
        QString                 m_cacheCurrentItem;
};


#endif /* AMAROK_COLLECTIONBROWSER_H */
