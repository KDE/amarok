
// (c) Pierpaolo Di Panfilo 2004
// See COPYING file for licensing information

#ifndef COVERMANAGER_H
#define COVERMANAGER_H

#include <qptrlist.h>
#include <qsplitter.h>
#include <kiconview.h>
#include <qdialog.h>
#include <qpixmap.h>

class QListViewItem;
class CoverViewItem;
class ClickLineEdit;
class KPushButton;
class KPopupMenu;
class QToolButton;
class QLabel;
class KListView;
class CoverView;
class QHBox;
class KProgress;
class QHBoxLayout;
class PixmapViewer;

class CoverManager : public QSplitter
{
        Q_OBJECT

        static CoverManager *s_instance;

    public:
        CoverManager();
       ~CoverManager();

        static CoverManager *instance() { return s_instance; }

        static void showOnce( const QString &artist = QString::null );
        static void viewCover( const QString& artist, const QString& album, QWidget *parent=0 );

        void setStatusText( QString text );

         /**
         * Return the top level domain for the current locale
         **/
        static QString amazonTld();
    public slots:
        void updateStatusBar();
        void changeLocale( int id );

    private slots:
        void slotArtistSelected( QListViewItem* );
        void coverItemExecuted( QIconViewItem *item );
        void showCoverMenu( QIconViewItem *item, const QPoint& );
        void slotSetFilter();
        void slotSetFilterTimeout();
        void changeView( int id );
        void fetchMissingCovers();
        void fetchCoversLoop();
        void coverFetched( const QString&, const QString& );
        void coverRemoved( const QString&, const QString& );
        void coverFetcherError();
        void stopFetching();

        void init();

    private:
        enum View { AllAlbums=0, AlbumsWithCover, AlbumsWithoutCover };

        void loadCover( const QString &, const QString & );
        void setCustomSelectedCovers();
        void fetchSelectedCovers();
        void deleteSelectedCovers();
        QPtrList<CoverViewItem> selectedItems();

        KListView      *m_artistView;
        CoverView      *m_coverView;
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
        QString         m_oldStatusText;

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

    private slots:
        void setStatusText( QIconViewItem *item );
};

class CoverViewItem : public KIconViewItem
{
    public:
        CoverViewItem( QIconView *parent, QIconViewItem *after, const QString &artist, const QString &album );

        void loadCover();
        bool hasCover() const;
        bool canRemoveCover() const { return !m_embedded && hasCover(); }
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
        bool    m_embedded;
};


class CoverViewDialog : public QDialog {
        Q_OBJECT

    public:
        CoverViewDialog(const QString& artist, const QString& album, QWidget *parent);

    private:
        QHBoxLayout *m_layout;
        QPixmap m_pixmap;
        PixmapViewer *m_pixmapViewer;
        QLabel *m_label;
};

#endif
