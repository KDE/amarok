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

#include "core/meta/forward_declarations.h"
#include "covermanager/CoverFetchUnit.h"
#include "network/NetworkAccessManagerProxy.h"
#include "widgets/BoxWidget.h"

#include <QLabel>
#include <QList>
#include <QListWidgetItem>
#include <QObject>
#include <QPointer>
#include <QProgressDialog>

class CoverFoundItem;
class CoverFoundSideBar;
class QDialog;
class KJob;
class KComboBox;
class QListWidget;
class QPushButton;
class BoxWidget;
class QTabWidget;

class CoverFoundDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CoverFoundDialog( const CoverFetchUnit::Ptr &unit,
                               const CoverFetch::Metadata &data = CoverFetch::Metadata(),
                               QWidget *parent = nullptr );
    ~CoverFoundDialog() override;

    /**
     * @returns the currently selected cover image
     */
    const QImage image() const { return m_image; }

    void setQueryPage( int page );

    const CoverFetchUnit::Ptr unit() const { return m_unit; }

Q_SIGNALS:
    void newCustomQuery( Meta::AlbumPtr album, const QString &query, int page );

public Q_SLOTS:
    void add( const QImage &cover,
              const CoverFetch::Metadata &metadata,
              const CoverFetch::ImageSize imageSize = CoverFetch::NormalSize );

protected:
    void hideEvent( QHideEvent *event ) override;

protected Q_SLOTS:
    void slotButtonClicked( int button );

private Q_SLOTS:
    void addToCustomSearch( const QString &text );
    void clearQueryButtonClicked();
    void clearView();
    void downloadProgressed( qint64 bytesReceived, qint64 bytesTotal );
    void fetchRequestRedirected( QNetworkReply *oldReply, QNetworkReply *newReply );
    void handleFetchResult( const QUrl &url, const QByteArray &data, const NetworkAccessManagerProxy::Error &e );
    void insertComboText( const QString &text );
    void currentItemChanged( QListWidgetItem *current, QListWidgetItem *previous );
    void itemDoubleClicked( QListWidgetItem *item );
    void itemMenuRequested( const QPoint &pos );
    void processCurrentQuery(); // Same as processQuery( QString() )
    void display(); ///< Opens a pixmap viewer
    void processQuery( const QString &input = QString() );
    void saveAs();
    void selectDiscogs();
    void selectLastFm();
    void selectGoogle();
    void sortingTriggered( bool checked );
    void updateSearchButton( const QString &text );

private:
    void addToView( CoverFoundItem *item );
    bool contains( const CoverFetch::Metadata &metadata ) const;
    bool fetchBigPix(); ///< returns true if full-size image is fetched successfully
    void sortCoversBySize();
    void updateGui();
    void updateTitle();

    CoverFoundSideBar *m_sideBar;     //!< View of selected cover and its metadata
    KComboBox *m_search;              //!< Custom search input
    QListWidget *m_view;              //!< View of retrieved covers
    QPushButton *m_save;              //!< Save Button
    QPushButton *m_searchButton;      //!< Button to start search or get more results for last query
    Meta::AlbumPtr m_album;           //!< Album associated with @ref m_unit;
    QAction *m_sortAction;            //!< Action to sort covers by size
    QList< int > m_sortSizes;         //!< List of sorted cover sizes used for indexing
    QImage m_image;                   //!< Currently selected cover image
    QString m_query;                  //!< Cache for the last entered custom query
    bool m_isSorted;                  //!< Are the covers sorted in the view?
    bool m_sortEnabled;               //!< Sort covers by size
    const CoverFetchUnit::Ptr m_unit; //!< Cover fetch unit that initiated this dialog
    int m_queryPage;                  //!< Cache for the page number associated with @ref m_query
    QHash<QUrl, CoverFoundItem*> m_urls; //!< Urls hash for network access manager proxy
    QPointer<QProgressDialog> m_dialog;  //!< Progress dialog for fetching big pix

    Q_DISABLE_COPY( CoverFoundDialog )
};

class CoverFoundSideBar : public BoxWidget
{
    Q_OBJECT

public:
    explicit CoverFoundSideBar( const Meta::AlbumPtr &album, QWidget *parent = nullptr );
    ~CoverFoundSideBar() override;

public Q_SLOTS:
    void clear();
    void setPixmap( const QPixmap &pixmap, const CoverFetch::Metadata &metadata );
    void setPixmap( const QPixmap &pixmap );

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

    Q_DISABLE_COPY( CoverFoundSideBar )
};

class CoverFoundItem : public QListWidgetItem
{
public:
    explicit CoverFoundItem( const QImage &cover,
                             const CoverFetch::Metadata &data,
                             const CoverFetch::ImageSize imageSize = CoverFetch::NormalSize,
                             QListWidget *parent = nullptr );
    ~CoverFoundItem() override;

    const CoverFetch::Metadata metadata() const { return m_metadata; }
    const QImage bigPix() const { return m_bigPix; }
    const QImage thumb() const { return m_thumb; }

    bool hasBigPix() const { return !m_bigPix.isNull(); }

    void setBigPix( const QImage &image ) { m_bigPix = image; }

    bool operator==( const CoverFoundItem &other ) const;
    bool operator!=( const CoverFoundItem &other ) const;

private:
    void setCaption();

    CoverFetch::Metadata m_metadata;
    QImage m_thumb;
    QImage m_bigPix;

    Q_DISABLE_COPY( CoverFoundItem )
};

#endif /* AMAROK_COVERFOUNDDIALOG_H */
