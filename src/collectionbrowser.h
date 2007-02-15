// (c) 2004 Mark Kretschmann <markey@web.de>
// (c) 2004 Christian Muehlhaeuser <chris@chris.de>
// (c) 2005 Gábor Lehel <illissius@gmail.com>
// (c) 2005 Christan Baumgart <christianbaumgart@web.de>
// See COPYING file for licensing information.

#ifndef AMAROK_COLLECTIONBROWSER_H
#define AMAROK_COLLECTIONBROWSER_H

#include <qlabel.h>
#include <qvaluelist.h>      //stack allocated
#include <qvbox.h>           //baseclass

#include <klistview.h>       //baseclass
#include <qstringlist.h>     //stack allocated
#include <kurl.h>            //stack allocated
#include <kdialogbase.h>     //baseclass
#include <kprogress.h>

#include "multitabbar.h"     //baseclass
#include "collectiondb.h"
#include "amarok_export.h"

class ClickLineEdit;
class CollectionDB;

class QCString;
class QDragObject;
class QPixmap;
class QPoint;
class QStringList;

class KAction;
class KComboBox;
class KPopupMenu;
class KRadioAction;
class KTabBar;
class KToolBar;
class KToggleAction;

class CollectionView;
class CollectionItem;
class DividerItem;
class OrganizeCollectionDialog;

namespace CollectionBrowserIds
{
    enum CatMenuId { IdAlbum = QueryBuilder::tabAlbum,
    IdArtist = QueryBuilder::tabArtist,
    IdComposer = QueryBuilder::tabComposer,
    IdGenre = QueryBuilder::tabGenre,
    IdYear = QueryBuilder::tabYear ,
    IdScan = 32, IdNone = 64,
    IdArtistAlbum = 128, IdGenreArtist = 256, IdGenreArtistAlbum = 512, IdVisYearAlbum = 1024, IdArtistVisYearAlbum = 2048,
    IdLabel = QueryBuilder::tabLabels //=8192
    };
}

class CollectionBrowser: public QVBox
{
    Q_OBJECT
    friend class CollectionView;

    public:
        CollectionBrowser( const char* name );
        virtual bool eventFilter( QObject*, QEvent* );
        KToolBar* getToolBar() const { return m_toolbar; }
        static CollectionBrowser *instance() { return s_instance; }

    public slots:
        void setupDirs();
        void toggleDivider();

    private slots:
        void slotClearFilter();
        void slotSetFilterTimeout();
        void slotSetFilter();
        void slotSetFilter( const QString &filter );
        void slotEditFilter();

    private:
        void layoutToolbar();
        void ipodToolbar( bool activate );
        void appendSearchResults();

        //attributes:
        KTabBar* m_tabs; //tree-view, flat-view tabs
        class KToolBar    *m_toolbar;
        KAction           *m_configureAction;
        // For iPod-style browsing
        KAction           *m_ipodIncrement, *m_ipodDecrement;
        class KToolBar    *m_ipodToolbar;
        class QHBox       *m_ipodHbox;

        KToggleAction     *m_showDividerAction;
        KRadioAction      *m_treeViewAction;
        KRadioAction      *m_flatViewAction;
        KRadioAction      *m_ipodViewAction;
        class KActionMenu *m_tagfilterMenuButton;

        KPopupMenu* m_categoryMenu;
        KPopupMenu* m_cat1Menu;
        KPopupMenu* m_cat2Menu;
        KPopupMenu* m_cat3Menu;
        KLineEdit*  m_searchEdit;
        KComboBox* m_timeFilter;
        CollectionView* m_view;
        QTimer*     m_timer;

        bool m_returnPressed;

        static CollectionBrowser *s_instance;

    // for CatMenuId
    friend class CollectionItem;
    friend class DividerItem;
};

class DividerItem : public KListViewItem
{
public:
    static QString createGroup(const QString& src, int cat);
    static bool shareTheSameGroup(const QString& a, const QString& b, int cat);

public:
    DividerItem( QListView* parent, QString txt, int cat);

    virtual void paintCell ( QPainter * p, const QColorGroup & cg, int column, int width, int align );
    virtual void paintFocus ( QPainter * p, const QColorGroup & cg, const QRect & r );

    virtual QString text(int column) const;

    void setBlockText(bool block) { m_blockText = block; }

private:
    virtual int compare( QListViewItem*, int, bool ) const;

private:
    bool m_blockText;
    QString m_text;
    int m_cat;
};



class CollectionItem : public KListViewItem {
    public:
        CollectionItem( QListView* parent, int cat = 0, bool unknown = false, bool sampler=false )
            : KListViewItem( parent )
            , m_cat( cat )
            , m_isUnknown( unknown )
            , m_isSampler( sampler ) {};
        CollectionItem( QListViewItem* parent, int cat = 0, bool unknown = false, bool sampler=false )
            : KListViewItem( parent )
            , m_cat( cat )
            , m_isUnknown( unknown )
            , m_isSampler( sampler ) {};
        void setUrl( const QString& url ) { m_url.setPath( url ); }
        const KURL& url() const { return m_url; }

        virtual void sortChildItems ( int column, bool ascending ); //reimplemented

        inline QString getSQLText( int column )
        {
            return ( !column && m_isUnknown ) ? "" : text( column );
        }

        inline bool isUnknown() {return m_isUnknown;}
        inline bool isSampler() {return m_isSampler;}

        virtual void setPixmap(int column, const QPixmap & pix);

        /// convenience functions
        CollectionView *listView() const { return reinterpret_cast<CollectionView*>( KListViewItem::listView() ); }

    private:
        friend class CollectionView;
        virtual void paintCell ( QPainter * painter, const QColorGroup & cg, int column, int width, int align );

        //for sorting
        virtual int compare( QListViewItem*, int, bool ) const; //reimplemented

    //attributes:
        KURL m_url;
        int m_cat;
        bool m_isUnknown;
        bool m_isSampler;
};


class CollectionView : public KListView, public DropProxyTarget
{
    Q_OBJECT
    friend class CollectionBrowser;

    public:
        enum ViewMode  { modeTreeView, modeFlatView, modeIpodView };

        friend class CollectionItem; // for access to m_cat2
        friend class ContextBrowser; // for setupDirs()

        CollectionView( CollectionBrowser* parent );
        ~CollectionView();

        LIBAMAROK_EXPORT static CollectionView* instance() { return m_instance; }

        void setFilter( const QString &filter )     { m_filter = filter; }
        void setTimeFilter( const uint timeFilter ) { m_timeFilter = timeFilter; }
        QString filter()                            { return m_filter; }
        uint    timeFilter()                        { return m_timeFilter; }
        CollectionItem* currentItem() { return static_cast<CollectionItem*>( KListView::currentItem() ); }

        int trackDepth() { return m_trackDepth; }
        int viewMode() const { return m_viewMode; }

        // Transform "The Who" -> "Who, The" or the other way
        static void manipulateThe( QString &str, bool reverse );

        void setShowDivider(bool show);

        bool isOrganizingFiles() const;

        //Useful helper function to avoid duplicating code
        static inline void yearAlbumCalc( QString &year, QString &text );

    protected:
        // Reimplemented for iPod-style navigation, etc.
        virtual void keyPressEvent( QKeyEvent *e );


    public slots:
        /** Rebuilds and displays the treeview by querying the database. */
        void renderView(bool force = false);

        void databaseChanged() { m_dirty = true; renderView(); };

        void setTreeMode() { setViewMode( modeTreeView ); };
        void setFlatMode() { setViewMode( modeFlatView ); };
        void setIpodMode() { setViewMode( modeIpodView ); };

        void presetMenu( int id );
        void cat1Menu( int id, bool rerender = true );
        void cat2Menu( int id, bool rerender = true );
        void cat3Menu( int id, bool rerender = true );
        void organizeFiles( const KURL::List &list, const QString &caption, bool addToCollection=false ) LIBAMAROK_EXPORT;

    private slots:
        void setupDirs();

        void slotEnsureSelectedItemVisible();

        void renderFlatModeView(bool force = false);
        void renderTreeModeView(bool force = false);
        void renderIpodModeView(bool force = false);

        void scanStarted();
        void scanDone( bool changed = true );

        void slotExpand( QListViewItem* );
        void slotCollapse( QListViewItem* );
        void enableCat3Menu( bool );
        void invokeItem( QListViewItem*, const QPoint &, int column );
        void invokeItem( QListViewItem* );

        // ipod-style navigation slots
        void ipodItemClicked( QListViewItem*, const QPoint&, int );
        void incrementDepth ( bool rerender = true );
        void decrementDepth ( bool rerender = true );

        void rmbPressed( QListViewItem*, const QPoint&, int );
        void selectAll() {QListView::selectAll(true); }
        /** Tries to download the cover image from Amazon.com */
        void fetchCover();
        /** Shows dialog with information on selected track */
        void showTrackInfo();

        /**
        * Cancel Organizing files
        */
        void cancelOrganizingFiles();

        void ratingChanged( const QString&, int );

    private:
        enum Tag { Title = 0, Artist, Composer, Album, Genre, Length, DiscNumber, Track, Year,
            Comment, Playcount, Score, Rating, Filename, Firstplay, Lastplay, Modified,
            Bitrate, Filesize, BPM, NUM_TAGS };

        void setViewMode( int mode, bool rerender = true );
        void removeDuplicatedHeaders();

        void startDrag();
        QString getTrueItemText( int, QListViewItem* ) const;
        QStringList listSelectedSiblingsOf( int, QListViewItem* );
        KURL::List listSelected();

        void playlistFromURLs( const KURL::List &urls );
        QPixmap iconForCategory( const int cat ) const;
        QString captionForCategory( const int cat ) const;
        inline QString captionForTag( const Tag ) const;

        // For iPod-style navigation
        QString allForCategory( const int cat, const int num ) const;
        void resetIpodDepth ( void );
        void buildIpodQuery ( QueryBuilder &qb, int depth, QStringList filters[3], QStringList filterYear, bool recursiveSort = false, bool compilationsOnly = false );
        void selectIpodItems ( void );
        QPixmap ipodIncrementIcon ( void );
        QPixmap ipodDecrementIcon ( void );

        void setCompilation( const KURL::List &urls, bool compilation );

        /** Rebuild selections, viewport and expanded items after reloads */
        void cacheView();
        void restoreView();

        //Used to store the name of an item (and its parents), so it can be recalled later
        //even if the pointer to the item has been invalidated.
        QStringList makeStructuredNameList( QListViewItem* ) const;
        QListViewItem* findFromStructuredNameList( const QStringList& ) const;

        // avoid duplicated code
        static inline bool endsInThe( const QString & text );
        inline void updateTrackDepth();

        uint translateTimeFilter( uint filterMode );

        /**Call when a category has changed **/
        void updateColumnHeader();
        // Reimplemented from KListView
        void viewportPaintEvent( QPaintEvent* );
        void viewportResizeEvent( QResizeEvent* );
        bool eventFilter( QObject*, QEvent* );
        void contentsDragEnterEvent( QDragEnterEvent* );
        void contentsDragMoveEvent( QDragMoveEvent* );
        void contentsDropEvent( QDropEvent *e );
        // Reimplemented from DropProxyTarget
        void dropProxyEvent( QDropEvent *e );

        void safeClear();

    //attributes:
        LIBAMAROK_EXPORT static CollectionView* m_instance;

        CollectionBrowser* m_parent;

        QString m_filter;
        uint m_timeFilter;
        int m_cat1;
        int m_cat2;
        int m_cat3;
        int m_trackDepth;
        int m_viewMode;

        // The iPod-style viewing attributes
        int         m_currentDepth;   // Current viewing depth
        QStringList m_ipodFilters[3]; // Selections at each stage
        QStringList m_ipodFilterYear; // See the comment for incrementDepth()
        // For auto-selection
        int         m_ipodIncremented; // 0 = nothing, 1 = just incremented, 2 = just decremented
        QStringList m_ipodSelected[3]; // Saved selections at lower levels
        QString     m_ipodCurrent[3];  // Saved current selections
        QString     m_ipodTopItem[3];  // Saved viewport positions

        bool m_dirty; // we use this to avoid re-rendering the view when unnecessary (eg, browser is not visible)

        QValueList<QStringList> m_cacheOpenItemPaths;
        QStringList             m_cacheViewportTopItem;
        QStringList             m_cacheCurrentItem;
        KURL::List              m_organizeURLs;
        bool                    m_organizeCopyMode;

        bool                    m_organizingFileCancelled;

        bool m_showDivider;
        QValueList<int>         m_flatColumnWidths;
};

// why is signal detailsClicked() missing from KDialogBase?
class OrganizeCollectionDialogBase : public KDialogBase
{
    Q_OBJECT
    public:
    OrganizeCollectionDialogBase( QWidget *parent=0, const char *name=0, bool modal=true,
            const QString &caption=QString::null,
            int buttonMask=Ok|Apply|Cancel )
        : KDialogBase( parent, name, modal, caption, buttonMask )
    {
    }

    signals:
        void detailsClicked();
    public slots:
        void slotDetails() { KDialogBase::slotDetails(); emit detailsClicked(); adjustSize(); }
};


#endif /* AMAROK_COLLECTIONBROWSER_H */
