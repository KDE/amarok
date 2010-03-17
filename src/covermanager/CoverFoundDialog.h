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
#include "meta/Meta.h"

#include <KDialog>
#include <KVBox>

#include <QLabel>
#include <QList>
#include <QListWidgetItem>
#include <QObject>
#include <QPixmap>

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
class QTableWidget;

class CoverFoundDialog : public KDialog
{
    Q_OBJECT

public:
    CoverFoundDialog( const CoverFetchUnit::Ptr unit,
                      const QPixmap cover = QPixmap(),
                      const CoverFetch::Metadata data = CoverFetch::Metadata(),
                      QWidget *parent = 0 );

    /**
    *   @returns the currently selected cover image
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
    void clearQueryButtonClicked();
    void clearView();
    void itemSelected();
    void itemDoubleClicked( QListWidgetItem *item );
    void itemMenuRequested( const QPoint &pos );
    void processQuery();
    void processQuery( const QString &query );
    void saveRequested();
    void selectDiscogs();
    void selectLastFm();
    void selectGoogle();
    void selectYahoo();
    void updateSearchButton( const QString &text );

private:
    void setupSearchToolTip();
    void updateGui();
    void updateTitle();

    KLineEdit      *m_search;        //! Custom search input
    KPushButton    *m_searchButton;  //! Button to start search or get more results for last query
    KListWidget    *m_view;          //! View of retreived covers
    KPushButton    *m_save;          //! Save Button
    CoverFoundSideBar *m_sideBar;    //! View of selected cover and its metadata
    QString         m_query;         //! Cache for the last entered custom query
    unsigned int    m_queryPage;     //! Cache for the page number associated with \ref m_query

    //! Cover fetch unit that initiated this dialog
    const CoverFetchUnit::Ptr m_unit;

    //! Currently selected cover image
    QPixmap m_pixmap;

    Q_DISABLE_COPY( CoverFoundDialog );
};

class CoverFoundSideBar : public KVBox
{
    Q_OBJECT

public:
    CoverFoundSideBar( QWidget *parent = 0 );
    ~CoverFoundSideBar();

public slots:
    void clear();
    void setPixmap( const QPixmap pixmap, CoverFetch::Metadata metadata );
    void setPixmap( const QPixmap pixmap );

private:
    QLabel               *m_notes;
    QLabel               *m_cover;
    QPixmap               m_pixmap;
    QTabWidget           *m_tabs;
    QTableWidget         *m_metaTable;
    CoverFetch::Metadata  m_metadata;

    void updateNotes();
    void updateMetaTable();

    QPixmap noCover( int size = 200 );
    QPixmap m_noCover; //! nocover cache

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

    void fetchBigPix();

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

    void slotFetchResult( KJob *job );

private:
    CoverFetch::Metadata m_metadata;
    QPixmap m_thumb;
    QPixmap m_bigPix;
    KDialog *m_dialog;
    KJobProgressBar *m_progress;
};

#endif /* AMAROK_COVERFOUNDDIALOG_H */
