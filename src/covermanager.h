// (c) Pierpaolo Di Panfilo 2004
// See COPYING file for licensing information

#ifndef COVERMANAGER_H
#define COVERMANAGER_H

#include <qdialog.h>
#include <kiconview.h>
#include <qptrlist.h>
#include <kio/previewjob.h>

class KLineEdit;
class KListView;
class KFileItem;
class KPopupMenu;
class KPushButton;

class QLabel;
class QListViewItem;
class QPixmap;
class QPoint;
class QTimer;

class CollectionDB;
class CoverViewItem;

class CoverManager : public QWidget
{
Q_OBJECT

    public:
        CoverManager( QWidget *parent=0, const char *name=0 );
        ~CoverManager();
        
        virtual bool eventFilter( QObject*, QEvent* );
     
    private slots:
        void expandeItem( QListViewItem * );
        void collapseItem( QListViewItem * );
        void slotArtistSelected( QListViewItem * );
        void coverItemDoubleClicked( QIconViewItem *item );
        void showCoverMenu( QIconViewItem *item, const QPoint & );
        void slotSetFilter();
        void slotSetFilterTimeout();
        void changeView( int id );
        void slotGotPreview( const KFileItem *, const QPixmap & );
        void previewJobFinished();
        void coverFetched( const QString & );
        void slotCoverDeleted();
        void fetchMissingCovers();
        void fetchMissingCoversLoop();
                
    private:
        enum { AllAlbums=0, AlbumsWithCover, AlbumsWithoutCover };
        
        void updateCounter();
        
        CollectionDB *m_db;
        KListView *m_artistView;
        KIconView *m_coverView;
        KLineEdit *m_searchEdit;
        KPopupMenu *m_viewMenu;
        KPushButton *m_fetchButton;
        QLabel *m_counterLabel;
        QTimer *m_timer;    //search filter timer
        QPtrList<KIconViewItem> m_coverItems;
        QString m_filter;
        int m_currentView;
        KIO::PreviewJob *m_previewJob;
        
        // Used by fetchMissingCovers() for temporary storage
        CoverViewItem* m_currentItem;
};


class CoverViewItem : public KIconViewItem
{
    public:
        CoverViewItem( QIconView *parent, QIconViewItem *after, QString artist, QString album );
        ~CoverViewItem();
        void updateCover( const QPixmap& );
        QString artist() { return m_artist; }
        QString album() { return m_album; }
        bool hasCover() { return m_hasCover; }
        QString albumPath();
        
    protected:
        void paintItem(QPainter* painter, const QColorGroup& colorGroup);
        void paintFocus(QPainter *, const QColorGroup &) { }
        void calcRect( const QString& text_=QString::null );
        
    private:
        QString m_artist;
        QString m_album;
        bool m_hasCover;
        bool noedit;
};


#endif
