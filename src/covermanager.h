// (c) Pierpaolo Di Panfilo 2004
// See COPYING file for licensing information

#ifndef COVERMANAGER_H
#define COVERMANAGER_H

#include "collectiondb.h"

#include <qdialog.h>
#include <qptrlist.h>
#include <kiconview.h>

class KListView;
class KPopupMenu;
class KProgress;
class KPushButton;

class QHBox;
class QLabel;
class QListViewItem;
class QPixmap;
class QPoint;
class QStringList;
class QTimer;
class QToolButton;

class ClickLineEdit;
class CollectionDB;
class CoverView;
class CoverViewItem;

class CoverManager : public QWidget
{
Q_OBJECT

    public:
        CoverManager();
       ~CoverManager();

        static void showOnce( const QString &artist = QString::null );
        static void viewCover( const QString& artist, const QString& album, QWidget *parent=0 );

    private slots:
        void expandItem( QListViewItem * );
        void collapseItem( QListViewItem * );
        void slotArtistSelected( QListViewItem * );

        void coverItemExecuted( QIconViewItem *item );
        void showCoverMenu( QIconViewItem *item, const QPoint & );
        //filter
        void slotSetFilter();
        void slotSetFilterTimeout();
        void changeView( int id );
        void changeLocale( int id );
        //cover fetching
        void fetchMissingCovers();
        void fetchCoversLoop();
        void coverFetched( const QString&, const QString& );
        void coverFetcherError();
        void stopFetching();
        void updateStatusBar();

        void init();

    private:
        enum View { AllAlbums=0, AlbumsWithCover, AlbumsWithoutCover };
        enum Locale { International=0, France, Germany, UK };

        void loadCover( const QString &, const QString & );
        void fetchSelectedCovers();
        void deleteSelectedCovers();
        QPtrList<CoverViewItem> selectedItems();

        KListView      *m_artistView;
        CoverView      *m_coverView;
        QHBox          *m_searchBox;
        ClickLineEdit  *m_searchEdit;
        KPushButton    *m_fetchButton;
        KPopupMenu     *m_amazonLocaleMenu;
        KPopupMenu     *m_viewMenu;
        QToolButton    *m_amazonLocaleButton;
        QToolButton    *m_viewButton;
        int             m_currentLocale;
        int             m_currentView;

        //status bar widgets
        QLabel         *m_statusLabel;
        QHBox          *m_progressBox;
        KProgress      *m_progress;

        QTimer         *m_timer;              //search filter timer
        QPtrList<QIconViewItem> m_coverItems; //used for filtering
        QString         m_filter;


        // Used by fetchCoversLoop() for temporary storage
        QStringList m_fetchCovers;
        uint m_fetchCounter;

        //used to display information about cover fetching in the status bar
        int m_fetchingCovers;
        int m_coversFetched;
        int m_coverErrors;
};

class CoverView : public KIconView
{
Q_OBJECT

    public:
        CoverView( QWidget *parent = 0, const char *name = 0, WFlags f = 0 );

    protected:
        QDragObject *dragObject();
    
    private:
};

class CoverViewItem : public KIconViewItem
{
    public:
        CoverViewItem( QIconView *parent, QIconViewItem *after, QString artist, QString album );

        void loadCover();
        bool hasCover() const;
        QString artist() const { return m_artist; }
        QString album() const { return m_album; }
        QPixmap coverPixmap() const { return m_coverPixmap; }

    protected:
        void paintItem(QPainter* painter, const QColorGroup& colorGroup);
        void paintFocus(QPainter *, const QColorGroup &) { }
        void dropped( QDropEvent *, const QValueList<QIconDragItem> & );
        void dragEntered();
        void dragLeft();
        void calcRect( const QString& text_=QString::null );

    private:
        QString m_artist;
        QString m_album;
        QString m_coverImagePath;
        QPixmap m_coverPixmap;
};


#endif
