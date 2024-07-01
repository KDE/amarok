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

#include "core/meta/Observer.h"
#include "covermanager/CoverFetcher.h"

#include <QAction>
#include <QDialog>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>

namespace Amarok { class LineEdit; }

class CompoundProgressBar;
class CoverViewItem;
class QTreeWidget;
class QTreeWidgetItem;
class KSqueezedTextLabel;
class QPushButton;
class QMenu;
class QLabel;
class CoverView;
class QHBoxLayout;
class QHBoxLayout;
class QColorGroup;
class QSplitter;

class CoverManager : public QDialog, public Meta::Observer
{
        Q_OBJECT

        static CoverManager *s_instance;
        static bool s_constructed;

    public:
        explicit CoverManager( QWidget *parent = nullptr );
       ~CoverManager() override;

        static bool isConstructed() { return s_constructed; }
        static CoverManager *instance() { return s_instance; }

        static void showOnce( const QString &artist = QString(), QWidget* parent = nullptr );
        static void viewCover( const Meta::AlbumPtr &album, QWidget* parent = nullptr );

        void setStatusText(const QString &text );

        // Reimplemented from Meta::Observer
        using Observer::metadataChanged;
        void metadataChanged( const Meta::AlbumPtr &album ) override;

    public Q_SLOTS:
        void updateStatusBar();
        void delayedDestruct();

    private:
        enum View { AllAlbums = 0, AlbumsWithCover, AlbumsWithoutCover };

    private Q_SLOTS:
        void slotArtistQueryResult( Meta::ArtistList artists );
        void slotContinueConstruction();

        void slotArtistSelected();
        void slotAlbumQueryResult( const Meta::AlbumList &albums );
        void slotAlbumFilterTriggered( QAction *action );
        void slotArtistQueryDone();
        void coverItemClicked( QListWidgetItem *item );
        void slotSetFilter();
        void slotSetFilterTimeout();

        void slotShowAllAlbums()          { changeView( AllAlbums );          }
        void slotShowAlbumsWithCover()    { changeView( AlbumsWithCover );    }
        void slotShowAlbumsWithoutCover() { changeView( AlbumsWithoutCover ); }
        void changeView( View id, bool force = false );

        void fetchMissingCovers();
        void updateFetchingProgress( int state );
        void stopFetching();

        void progressAllDone();
        void cancelCoverViewLoading();

    private:
        void loadCover( const QString &, const QString & );

        QSplitter        *m_splitter;
        QTreeWidget      *m_artistView;
        CoverView        *m_coverView;

        //hack to have something to show while the real list is hidden when loading thumbnails
        CoverView        *m_coverViewSpacer;
        Amarok::LineEdit *m_searchEdit;
        QPushButton      *m_fetchButton;
        QPushButton      *m_viewButton;
        QMenu            *m_viewMenu;
        View              m_currentView;

        Meta::ArtistList m_artistList;
        QList< QTreeWidgetItem* > m_items;
        Meta::AlbumList m_albumList;

        CoverFetcher   *m_fetcher;

        QAction        *m_selectAllAlbums;
        QAction        *m_selectAlbumsWithCover;
        QAction        *m_selectAlbumsWithoutCover;

        //status bar widgets
        CompoundProgressBar *m_progress;
        KSqueezedTextLabel *m_statusLabel;
        QString         m_oldStatusText;

        QTimer         *m_timer;              //search filter timer
        QList<CoverViewItem*> m_coverItems; //used for filtering
        QString         m_filter;


        // Used by fetchCoversLoop() for temporary storage
        Meta::AlbumList m_fetchCovers;

        //used to display information about cover fetching in the status bar
        bool m_fetchingCovers;
        int m_coversFetched;
        int m_coverErrors;

        bool m_isLoadingCancelled;
};

class CoverView : public QListWidget
{
    Q_OBJECT

    public:
        explicit CoverView( QWidget *parent = nullptr, const char *name = nullptr, Qt::WindowFlags f = {} );

    protected:
        void contextMenuEvent( QContextMenuEvent *event ) override;

    private Q_SLOTS:
        void setStatusText( QListWidgetItem *item );
};

class CoverViewItem : public QListWidgetItem
{
    public:
        CoverViewItem( QListWidget *parent, Meta::AlbumPtr album );
        ~CoverViewItem() override;

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
};

#endif
