/*
  Copyright (c) 2006  Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.
*/

#ifndef AMAROKMAGNATUNEBROWSER_H
#define AMAROKMAGNATUNEBROWSER_H


#include "amarok.h"
#include "magnatuneartistinfobox.h"
#include "magnatunelistview.h"
#include "magnatunelistviewitems.h"
#include "magnatunepurchasedialog.h"
#include "magnatunepurchasehandler.h"
#include "magnatunexmlparser.h"

#include <kio/job.h>
#include <kio/jobclasses.h>

#include <qcheckbox.h>
#include <qcombobox.h>
#include <qhbox.h>
#include <qpopupmenu.h>
#include <qpushbutton.h>
#include <qvbox.h>



/**
Amarok browser that displays all the music available at magnatune.com and makes it available for previewing and purchasing

@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class MagnatuneBrowser : public QVBox
{
    Q_OBJECT

public:
    ~MagnatuneBrowser() { }

    static MagnatuneBrowser *instance() {
        if(!s_instance)  s_instance = new MagnatuneBrowser("MagnatuneBrowser");
        return s_instance;
    }

private slots:
    void menuAboutToShow();
    void purchaseButtonClicked();
    void purchaseSelectedAlbum();
    void purchaseAlbumContainingSelectedTrack();
    void addSelectionToPlaylist();
    void itemExecuted( QListViewItem * );
    void selectionChanged( QListViewItem * );
    void showPopupMenu( QListViewItem * item, const QPoint & pos, int column );
    void updateButtonClicked();
    void showInfo(bool show);
    void listDownloadComplete( KIO::Job* downLoadJob);
    void genreChanged();
    void doneParsing();

private:
    MagnatuneBrowser( const char *name );

    void initBottomPanel();
    void initTopPanel();

    bool updateMagnatuneList();

    void addTrackToPlaylist ( MagnatuneTrack  *item );
    void addAlbumToPlaylist ( MagnatuneAlbum  *item );
    void addArtistToPlaylist( MagnatuneArtist *item );

    void updateList();
    void updateGenreBox();

    void startDrag();

    static MagnatuneBrowser *s_instance;

    MagnatuneListView         *m_listView;
    MagnatuneArtistInfoBox    *m_artistInfobox;
    QString                    m_currentInfoUrl;
    QPopupMenu                *m_popupMenu;
    MagnatunePurchaseHandler  *m_purchaseHandler;

    QHBox       *m_topPanel;
    QVBox       *m_bottomPanel;
    QPushButton *m_updateListButton;
    QPushButton *m_purchaseAlbumButton;
    QPushButton *m_showInfoToggleButton;

    QComboBox   *m_genreComboBox;
    bool         m_isInfoShown;

    KIO::TransferJob * m_listDownloadJob;
};


#endif
