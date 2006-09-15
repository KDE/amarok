// Author: Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>, (C) 2006
//
// Copyright: See COPYING file that comes with this distribution

#ifndef AMAROKMAGNATUNEBROWSER_H
#define AMAROKMAGNATUNEBROWSER_H


#include "magnatunelistview.h"
#include "magnatunelistviewitems.h"
#include "magnatunexmlparser.h"
#include "magnatuneartistinfobox.h"
#include "magnatunepurchasedialog.h"
#include "magnatunepurchasehandler.h"
#include "amarok.h"


#include <kio/job.h>
#include <kio/jobclasses.h>

#include <qvbox.h>
#include <qhbox.h>
#include <qpopupmenu.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qcombobox.h>




/**
A first attempt at making a browser that displays all the music available at magnatune.com

@author Nikolaj Hald Nielsen
*/
class MagnatuneBrowser : public QVBox
{

Q_OBJECT
public:
    MagnatuneBrowser( const char *name );

    ~MagnatuneBrowser();

     

protected:

    
    
    MagnatuneListView * m_listView;
    MagnatuneArtistInfoBox * m_artistInfobox;
    QString m_currentInfoUrl;
    QPopupMenu * m_popupMenu;
    MagnatunePurchaseHandler * m_purchaseHandler;

    QHBox * m_topPanel;
    QHBox * m_bottomPanel;
    QPushButton * m_updateListButton;
    QCheckBox * m_showInfoCheckbox;

    QComboBox * m_genreComboBox;
    bool isInfoShown;

    KIO::TransferJob * m_listDownloadJob;

    void initBottomPanel();
    void initTopPanel();

    bool updateMagnatuneList();

    void addTrackToPlaylist(MagnatuneTrack * item);
    void addAlbumToPlaylist(MagnatuneAlbum * item);
    void addArtistToPlaylist(MagnatuneArtist * item);


    void updateList();
    void updateGenreBox();

    void startDrag();


protected slots:

    void menuAboutToShow();
    void purchaseAlbum();
    void addSelectionToPlaylist();
    void itemExecuted(QListViewItem *);
    void selectionChanged(QListViewItem *);
    void showPopupMenu(QListViewItem * item, const QPoint & pos, int column);
    void updateButtonClicked();
    void showInfoCheckBoxStateChanged();
    void listDownloadComplete( KIO::Job* downLoadJob);
    void genreChanged();
    void  DoneParsing();



};


#endif
