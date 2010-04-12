/****************************************************************************************
 * Copyright (c) 2004 Mark Kretschmann <kretschmann@kde.org>                            *
 * Copyright (c) 2004 Stefan Bogner <bochi@online.ms>                                   *
 * Copyright (c) 2007 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
 * Copyright (c) 2009 Martin Sandsmark <sandsmark@samfundet.no>                         *
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

#ifndef AMAROK_COVERFOUNDDIALOG_H
#define AMAROK_COVERFOUNDDIALOG_H

#include "CoverFetchUnit.h"
#include "core/meta/Meta.h"

#include <KDialog>
#include <KVBox>

#include <QLabel>
#include <QList>
#include <QListWidgetItem>
#include <QObject>
#include <QPixmap>
#include <QPointer>

class CoverFoundItem;
class CoverFoundSideBar;
class KDialog;
class KJob;
class KJobProgressBar;
class KLineEdit;
class KListWidget;
class KPushButton;
class QFrame;
class QGridLayout;
class QTabWidget;

class CoverFoundDialog : public KDialog
{
    Q_OBJECT

public:
    explicit CoverFoundDialog( const CoverFetchUnit::Ptr unit,
                               const QPixmap cover = QPixmap(),
                               const CoverFetch::Metadata data = CoverFetch::Metadata(),
                               QWidget *parent = 0 );
    ~CoverFoundDialog();

    /**
     * @returns the currently selected cover image
     */
    const QPixmap image() const { return m_pixmap; }

    const CoverFetchUnit::Ptr unit() const { return m_unit; }

signals:
    void newCustomQuery( const QString &query, unsigned int page );

public slots:
    void add( const QPixmap cover,
              const CoverFetch::Metadata metadata,
              const CoverFetch::ImageSize imageSize = CoverFetch::NormalSize );

protected:
    void hideEvent( QHideEvent *event );

private slots:
    void addToCustomSearch( const QString &text );
    void clearQueryButtonClicked();
    void clearView();
    void itemSelected();
    void itemDoubleClicked( QListWidgetItem *item );
    void itemMenuRequested( const QPoint &pos );
    void processQuery();
    void processQuery( const QString &query );
    void saveAs();
    void saveRequested();
    void selectDiscogs();
    void selectLastFm();
    void selectGoogle();
    void selectYahoo();
    void sortingTriggered( bool checked );
    void updateSearchButton( const QString &text );

private:
    void addToView( CoverFoundItem *const item );
    void setupSearchToolTip();
    void sortCoversBySize();
    void updateGui();
    void updateTitle();

    CoverFoundSideBar *m_sideBar;     //!< View of selected cover and its metadata
    KLineEdit *m_search;              //!< Custom search input
    KListWidget *m_view;              //!< View of retrieved covers
    KPushButton *m_save;              //!< Save Button
    KPushButton *m_searchButton;      //!< Button to start search or get more results for last query
    Meta::AlbumPtr m_album;           //!< Album associated with @ref m_unit;
    QAction *m_sortAction;            //!< Action to sort covers by size
    QList< int > m_sortSizes;         //!< List of sorted cover sizes used for indexing
    QPixmap m_pixmap;                 //!< Currently selected cover image
    QString m_query;                  //!< Cache for the last entered custom query
    bool m_isSorted;                  //!< Are the covers sorted in the view?
    bool m_sortEnabled;               //!< Sort covers by size
    const CoverFetchUnit::Ptr m_unit; //!< Cover fetch unit that initiated this dialog
    unsigned int m_queryPage;         //!< Cache for the page number associated with @ref m_query

    Q_DISABLE_COPY( CoverFoundDialog );
};

class CoverFoundSideBar : public KVBox
{
    Q_OBJECT

public:
    explicit CoverFoundSideBar( const Meta::AlbumPtr album, QWidget *parent = 0 );
    ~CoverFoundSideBar();

public slots:
    void clear();
    void setPixmap( const QPixmap pixmap, CoverFetch::Metadata metadata );
    void setPixmap( const QPixmap pixmap );

private:
    Meta::AlbumPtr        m_album;
    QLabel               *m_notes;
    QLabel               *m_cover;
    QPixmap               m_pixmap;
    QTabWidget           *m_tabs;
    QWidget              *m_metaTable;
    CoverFetch::Metadata  m_metadata;

    void updateNotes();
    void updateMetaTable();
    void clearMetaTable();

    Q_DISABLE_COPY( CoverFoundSideBar );
};

class CoverFoundItem : public QObject, public QListWidgetItem
{
    Q_OBJECT

public:
    explicit CoverFoundItem( const QPixmap cover,
                             const CoverFetch::Metadata data,
                             const CoverFetch::ImageSize imageSize = CoverFetch::NormalSize,
                             QListWidget *parent = 0 );
    ~CoverFoundItem();

    bool fetchBigPix(); ///< returns true if full-size image is fetched successfully

    const CoverFetch::Metadata metadata() const { return m_metadata; }
    const QPixmap bigPix() const { return m_bigPix; }
    const QPixmap thumb() const { return m_thumb; }

    bool hasBigPix() const { return !m_bigPix.isNull(); }

    void setBigPix( const QPixmap &pixmap ) { m_bigPix = pixmap; }

signals:
    void pixmapChanged( const QPixmap pixmap );

public slots:
    /**
     * Opens a pixmap viewer
     */
    void display();

    /**
     * saveAs opens a dialog for choosing where to save the pixmap
     * @param album used for dialog's start url
     */
    void saveAs( Meta::AlbumPtr album );

    void slotFetchResult( KJob *job );

private:
    void setCaption();

    CoverFetch::Metadata m_metadata;
    QPixmap m_thumb;
    QPixmap m_bigPix;
    QPointer<KDialog> m_dialog;
    QPointer<KJobProgressBar> m_progress;
};

#endif /* AMAROK_COVERFOUNDDIALOG_H */
