/****************************************************************************************
 * Copyright (c) 2004 Pierpaolo Di Panfilo <pippo_dp@libero.it>                         *
 * Copyright (c) 2007 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef COVERMANAGER_H
#define COVERMANAGER_H

#include "CoverFetcher.h"

#include <QSplitter>
#include <QDropEvent>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QAction>

namespace Amarok { class LineEdit; }

class CoverViewItem;
class QTreeWidget;
class QTreeWidgetItem;
class KPushButton;
class KMenu;
class QToolButton;
class QLabel;
class CoverView;
class KHBox;
class QProgressBar;
class QHBoxLayout;
class QColorGroup;
class QProgressDialog;

class CoverManager : public QSplitter, public Meta::Observer
{
        Q_OBJECT

        static CoverManager *s_instance;
        static bool s_constructed;

    public:
        CoverManager();
       ~CoverManager();

        static bool isConstructed() { return s_constructed; }
        static CoverManager *instance() { return s_instance; }

        static void showOnce( const QString &artist = QString() );
        static void viewCover( Meta::AlbumPtr album, QWidget *parent=0 );

        void setStatusText( QString text );

        // Reimplemented from Meta::Observer
        using Observer::metadataChanged;
        void metadataChanged( Meta::AlbumPtr album );

    public slots:
        void updateStatusBar();

    private slots:
        void slotArtistQueryResult( QString collectionId, Meta::ArtistList artists );
        void slotContinueConstruction();
        void init();

        void slotArtistSelected();
        void slotArtistSelectedContinue();
        void slotAlbumQueryResult( QString collectionId, Meta::AlbumList albums );
        void slotArtistSelectedContinueAgain();
        void coverItemExecuted( QListWidgetItem *item );
        void slotSetFilter();
        void slotSetFilterTimeout();

        void slotShowAllAlbums()          { changeView( AllAlbums );          }
        void slotShowAlbumsWithCover()    { changeView( AlbumsWithCover );    }
        void slotShowAlbumsWithoutCover() { changeView( AlbumsWithoutCover ); }
        void changeView( int id );
        
        void fetchMissingCovers();
        void coverFetched( const QString&, const QString& );
        void coverRemoved( const QString&, const QString& );
        void coverFetcherError();
        void stopFetching();

        void playSelectedAlbums();

    private:
        enum View { AllAlbums=0, AlbumsWithCover, AlbumsWithoutCover };

        void loadCover( const QString &, const QString & );
        QList<CoverViewItem*> selectedItems();

        QTreeWidget      *m_artistView;
        CoverView        *m_coverView;

        //hack to have something to show while the real list is hidden when loading thumbnails
        CoverView        *m_coverViewSpacer;
        Amarok::LineEdit *m_searchEdit;
        KPushButton      *m_fetchButton;
        KMenu            *m_viewMenu;
        QToolButton      *m_viewButton;
        int               m_currentView;

        Meta::ArtistList m_artistList;
        QList< QTreeWidgetItem* > m_items;
        Meta::AlbumList m_albumList;

        QProgressDialog* m_progressDialog;
        
        CoverFetcher   *m_fetcher;

        QAction        *m_selectAllAlbums;
        QAction        *m_selectAlbumsWithCover;
        QAction        *m_selectAlbumsWithoutCover;

        //status bar widgets
        QLabel         *m_statusLabel;
        KHBox          *m_progressBox;
        QProgressBar   *m_progress;
        QString         m_oldStatusText;

        QTimer         *m_timer;              //search filter timer
        QList<CoverViewItem*> m_coverItems; //used for filtering
        QString         m_filter;


        // Used by fetchCoversLoop() for temporary storage
        Meta::AlbumList m_fetchCovers;

        //used to display information about cover fetching in the status bar
        int m_fetchingCovers;
        int m_coversFetched;
        int m_coverErrors;
};

class CoverView : public QListWidget
{
    Q_OBJECT

    public:
        explicit CoverView( QWidget *parent = 0, const char *name = 0, Qt::WFlags f = 0 );

    protected:
        void contextMenuEvent( QContextMenuEvent *event );

    private slots:
        void setStatusText( QListWidgetItem *item );
};

class CoverViewItem : public QListWidgetItem
{
    public:
        CoverViewItem( QListWidget *parent, Meta::AlbumPtr album );
        ~CoverViewItem();

        void loadCover();
        bool hasCover() const;
        bool canRemoveCover() const { return !m_embedded && hasCover(); }
        QString artist() const { return m_artist; }
        QString album() const { return m_album; }
        Meta::AlbumPtr albumPtr() const { return m_albumPtr; }

    protected:
        void paintFocus(QPainter *, const QColorGroup &) { }
        void dragEntered();
        void dragLeft();

    private:
        Meta::AlbumPtr m_albumPtr;
        QString m_artist;
        QString m_album;
        QString m_coverImagePath;
        bool    m_embedded;
        QListWidget *m_parent;
};

/**
*   Wrapper class to connect multiple actions to a single entry
*/
class MultipleAction : public QAction
{
    Q_OBJECT
    public:
        MultipleAction( QObject *parent, QList<QAction *> actions )
            : QAction( parent )
        {
            foreach( QAction *action, actions )
                connect( this, SIGNAL( triggered( bool ) ), action, SLOT( trigger() ) );

            if ( actions.count() > 0 )
            {
                setText( actions.first()->text() );
                setIcon( actions.first()->icon() );
                setToolTip( actions.first()->toolTip() );
            }
        }
};

#endif
