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

#include <QLabel>
#include <QList>
#include <QListWidgetItem>
#include <QObject>
#include <QPixmap>

class KDialog;
class KJob;
class KJobProgressBar;
class KLineEdit;
class KListWidget;
class KPushButton;
class QFrame;
class QGridLayout;

class CoverFoundDialog : public KDialog
{
    Q_OBJECT

public:
    CoverFoundDialog( Meta::AlbumPtr album,
                      const QPixmap cover = QPixmap(),
                      const CoverFetch::Metadata data = CoverFetch::Metadata(),
                      QWidget *parent = 0 );

    /**
    *   @returns the currently selected cover image
    */
    const QPixmap image() { return m_pixmap; }

signals:
    void newCustomQuery( const QString & );

public slots:
    void add( const QPixmap cover,
              const CoverFetch::Metadata metadata,
              const CoverFetch::ImageSize imageSize = CoverFetch::NormalSize );

protected:
    void hideEvent( QHideEvent *event );

private slots:
    void clearView();
    void itemClicked( QListWidgetItem *item );
    void itemDoubleClicked( QListWidgetItem *item );
    void itemMenuRequested( const QPoint &pos );
    void saveRequested();
    void searchButtonPressed();
    void selectLastFmSearch();
    void selectWebSearch();

private:
    void updateGui();
    void updateDetails();
    void updateTitle();

    QFrame         *m_details;       //! Details widget
    QGridLayout    *m_detailsLayout; //! Details widget layout
    KLineEdit      *m_search;        //! Custom search input
    KListWidget    *m_view;          //! View of retreived covers
    KPushButton    *m_save;          //! Save Button

    //! Album associated with the covers
    Meta::AlbumPtr m_album;

    //! Currently selected cover image
    QPixmap m_pixmap;

    Q_DISABLE_COPY( CoverFoundDialog );
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
