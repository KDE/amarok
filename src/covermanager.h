// (c) Pierpaolo Di Panfilo 2004
// See COPYING file for licensing information

#ifndef COVERMANAGER_H
#define COVERMANAGER_H

#include <qdialog.h>
#include <kiconview.h>
#include <qptrlist.h>

class KLineEdit;
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

class CollectionDB;
class CoverViewItem;

class CoverManager : public QWidget
{
Q_OBJECT

    public:
        CoverManager( QWidget *parent=0, const char *name=0 );
        ~CoverManager();

    private slots:
        void expandItem( QListViewItem * );
        void collapseItem( QListViewItem * );
        void slotArtistSelected( QListViewItem * );
        void loadThumbnails();

        void coverItemDoubleClicked( QIconViewItem *item );
        void showCoverMenu( QIconViewItem *item, const QPoint & );
        //filter
        void slotSetFilter();
        void slotSetFilterTimeout();
        void clearFilter();
        void changeView( int id );
        //cover fetching
        void fetchMissingCovers();
        void fetchCoversLoop();
        void coverFetched( const QString &, const QString & );
        void coverFetcherError();
        void stopFetching();
        void updateStatusBar();

    private:
        enum View { AllAlbums=0, AlbumsWithCover, AlbumsWithoutCover };

        void loadCover( const QString &, const QString & );
        void fetchSelectedCovers();
        void deleteSelectedCovers();
        QPtrList<CoverViewItem> selectedItems();
        bool eventFilter( QObject*, QEvent* );

        CollectionDB *m_db;

        KListView *m_artistView;
        KIconView *m_coverView;
        KLineEdit *m_searchEdit;
        QToolButton *m_viewButton;
        KPopupMenu *m_viewMenu;
        KPushButton *m_fetchButton;
        //status bar widgets
        QLabel *m_statusLabel;
        QHBox *m_progressBox;
        KProgress *m_progress;

        QTimer *m_timer;    //search filter timer
        QPtrList<QIconViewItem> m_coverItems; //used for filtering
        QString m_filter;
        int m_currentView;

        //used for the thumbnail loading
        QStringList m_loadAlbums;
        bool m_stopLoading;

        // Used by fetchCoversLoop() for temporary storage
        QStringList m_fetchCovers;
        uint m_fetchCounter;

        //used to display information about cover fetching in the status bar
        int m_fetchingCovers;
        int m_coversFetched;
        int m_coverErrors;
};


class CoverViewItem : public KIconViewItem
{
    public:
        CoverViewItem( QIconView *parent, QIconViewItem *after, QString artist, QString album );

        void loadCover();
        bool hasCover() { return m_hasCover; }
        const QString artist() { return m_artist; }
        const QString album() { return m_album; }
        const QString coverImagePath();

    protected:
        void paintItem(QPainter* painter, const QColorGroup& colorGroup);
        void paintFocus(QPainter *, const QColorGroup &) { }
        void calcRect( const QString& text_=QString::null );

    private:
        QString m_artist;
        QString m_album;
        bool m_hasCover;
        QPixmap m_coverPix;
};


#endif
