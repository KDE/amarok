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

#include "meta/Meta.h"

#include <KDialog>
#include <KPushButton>

#include <QHash>
#include <QLabel>
#include <QList>
#include <QObject>
#include <QPixmap>

class KHBox;
class KLineEdit;
class KPushButton;
class QGridLayout;

class CoverFoundDialog : public KDialog
{
    Q_OBJECT

public:
    explicit CoverFoundDialog( QWidget *parent,
                               Meta::AlbumPtr album = KSharedPtr< Meta::Album >(),
                               const QList< QPixmap > &covers = QList< QPixmap >() );

    /**
    *   @returns the currently selected cover image
    */
    const QPixmap image() { return *m_labelPixmap->pixmap(); }

signals:
    void newCustomQuery( const QString & );

public slots:
    virtual void accept();

    void add( QPixmap cover );
    void add( QList< QPixmap > covers );

protected:
    void keyPressEvent( QKeyEvent *event );
    void resizeEvent( QResizeEvent *event );
    void closeEvent( QCloseEvent *event );
    void wheelEvent( QWheelEvent *event );

private slots:
    /**
    *   Switch picture label and current index to next cover
    */
    void nextPix();

    /**
    *   Switch picture label and current index to previous cover
    */
    void prevPix();

private:
    void updateGui();
    void updatePixmap();
    void updateButtons();
    void updateDetails();
    void updateTitle();

    QPixmap noCover( int size = 300 );
    QPixmap m_noCover;               //! nocover.png cache

    QLabel         *m_labelPixmap;   //! Pixmap container
    QFrame         *m_details;       //! Details widget
    QGridLayout    *m_detailsLayout; //! Details widget layout
    KLineEdit      *m_search;        //! Custom search input
    KPushButton    *m_next;          //! Next Button
    KPushButton    *m_prev;          //! Back Button
    KPushButton    *m_save;          //! Save Button

    //! Album associated with the covers
    Meta::AlbumPtr m_album;

    //! Retrieved covers for the album
    QList< QPixmap > m_covers;

    //! Current position indices for m_covers
    int m_index;

    Q_DISABLE_COPY( CoverFoundDialog );
};

#endif /* AMAROK_COVERFOUNDDIALOG_H */
