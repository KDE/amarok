/***************************************************************************
 *   Copyright (c) 2006, 2007                                              *
 *        Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02111-1307, USA.          *
 ***************************************************************************/

#ifndef AMAROKMAGNATUNESTORE_H
#define AMAROKMAGNATUNESTORE_H


#include "amarok.h"
//#include "magnatunepurchasedialog.h"
#include "magnatunepurchasehandler.h"
#include "magnatuneredownloadhandler.h"
#include "magnatunexmlparser.h"
#include "magnatunedatabasehandler.h"

#include "../servicebase.h"
#include "servicesqlcollection.h"

#include <kio/job.h>
#include <kio/jobclasses.h>

#include <QCheckBox>
#include <QComboBox>
#include <khbox.h>
#include <q3popupmenu.h>
#include <QPushButton>
#include <kvbox.h>



/**
Amarok browser that displays all the music available at magnatune.com and makes it available for previewing and purchasing.
Implemented as a singleton

@author Nikolaj Hald Nielsen <nhnFreespirit@gmail.com>
*/
class MagnatuneStore: public ServiceBase
{
    Q_OBJECT

public:

     /**
     * Constructor
     */
    MagnatuneStore( const char *name );
    /**
     * Destructor
     */
    ~MagnatuneStore() { }

     /**
     * Do not do expensive initializations before we are actually shown
     */
    void polish();
   // bool updateContextView();

private slots:

    /**
     * Slot called when the purchase album button is clicked. Starts a purchase
     */
    void purchaseButtonClicked();

    /**
     * Slot for recieving notification that the update button has been clicked.
     */
    void updateButtonClicked();


    /**
     * Slot for recieving notification when the Magnatune xml file has been downloaded.
     * Triggers a parse of the file to get the info added to the databse
     * @param downLoadJob The calling download Job
     */
    void listDownloadComplete( KJob* downLoadJob);

    /**
     * Slot for catching cancelled list downloads
     */
    void listDownloadCancelled();

    /**
     * Slot called when the parsing of the Magnatuin xml file is completed.
     * Triggers an update of the list view and the genre combo box
     */
    void doneParsing();

    /**
     * Starts the process of redownloading a previously bought album
     */
    //void processRedownload();

    /**
     * Slot for recieving notifications of completed purchase operations
     * @param success Was the operation a success?
     */
    void purchaseCompleted( bool success );


    /**
     * Adds all tracks with a common mood to the playlist
     * @param mood The mood of the tracks to add
     */
    //void addMoodyTracksToPlaylist( QString mood );


     /**
     * Checks if purchase button should be enabled
     * @param selection the new selection
     * @param deseleted items that were previously selected but have been deselected
     */
    void itemSelected( CollectionTreeItem * selectedItem );

private:
    /**
     * Helper function that initializes the button panel below the list view
     */
    void initBottomPanel();

    /**
     * Helper function that initializes the genre selection panel above the list view
     */
    void initTopPanel();

    /**
     * Starts downloading an updated track list xml file from
     * http://magnatune.com/info/album_info.xml
     * @return Currently always returns true
     */
    bool updateMagnatuneList();

    /**
     * Adds a magnatune preview track to the playlist.
     * @param item The track to add
     */
    void addTrackToPlaylist ( Meta::MagnatuneTrack  *item );

    static MagnatuneStore *s_instance;

    QString                    m_currentInfoUrl;
    QMenu                *m_popupMenu;
    MagnatunePurchaseHandler  *m_purchaseHandler;
    //MagnatuneRedownloadHandler *m_redownloadHandler;

    QPushButton *m_advancedFeaturesButton;
    QPushButton *m_updateListButton;
    QPushButton *m_purchaseAlbumButton;
    QPushButton *m_showInfoToggleButton;

    QComboBox   *m_genreComboBox;
    bool         m_purchaseInProgress;

    Meta::MagnatuneAlbum * m_currentAlbum;


    KIO::FileCopyJob * m_listDownloadJob;

    ServiceSqlCollection * m_collection;

};


#endif
