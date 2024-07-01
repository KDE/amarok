/****************************************************************************************
 * Copyright (c) 2006,2007 Nikolaj Hald Nielsen <nhn@kde.org>                           *
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

#ifndef AMAROKMAGNATUNESTORE_H
#define AMAROKMAGNATUNESTORE_H


#include "core/support/Amarok.h"
#include "MagnatuneDownloadHandler.h"
#include "MagnatuneRedownloadHandler.h"
#include "MagnatuneXmlParser.h"
#include "MagnatuneDatabaseHandler.h"
#include "MagnatuneSqlCollection.h"

#include "../ServiceBase.h"

#include <KJob>

#include <QPushButton>


class MagnatuneInfoParser;
class MagnatuneNeedUpdateWidget;

class MagnatuneServiceFactory : public ServiceFactory
{
    Q_PLUGIN_METADATA(IID AmarokPluginFactory_iid FILE "amarok_service_magnatunestore.json")
    Q_INTERFACES(Plugins::PluginFactory)
    Q_OBJECT

    public:
        MagnatuneServiceFactory();
        ~MagnatuneServiceFactory() override {}

        void init() override;
        QString name() override;
        KConfigGroup config() override;

        bool possiblyContainsTrack( const QUrl &url ) const override { return url.url().contains( QStringLiteral("magnatune.com"), Qt::CaseInsensitive ); }
};


/**
Amarok browser that displays all the music available at magnatune.com and makes it available for previewing and purchasing.
Implemented as a singleton

@author Nikolaj Hald Nielsen <nhn@kde.org>
*/
class MagnatuneStore: public ServiceBase
{
    Q_OBJECT

public:
     /**
     * Constructor
     */
    explicit MagnatuneStore( MagnatuneServiceFactory* parent, const char *name );
    /**
     * Destructor
     */
    ~MagnatuneStore() override;

    void setMembership( int type, const QString &username,  const QString &password );

    /**
     * OGG, MP3 or LOFI
     */
    void setStreamType( int );
    
     /**
     * Do not do expensive initializations before we are actually shown
     */
    void polish() override;
   // bool updateContextView();

    Collections::Collection * collection() override { return m_collection; }

    QString messages() override;
    QString sendMessage( const QString &message ) override;

public Q_SLOTS:
    /**
    * Slot for catching cancelled list downloads
    */
    void listDownloadCancelled();

    void downloadTrack( Meta::MagnatuneTrack * track );

    void downloadAlbum( Meta::MagnatuneAlbum * album );

    void showFavoritesPage();
    void showHomePage();
    void showRecommendationsPage();

    void addToFavorites( const QString &sku );
    void removeFromFavorites( const QString &sku );
    
private Q_SLOTS:
    /**
     * Slot called when the download album button is clicked. Starts a download
     */
    void download();

    void downloadSku( const QString &sku );

    /**
     * Slot for receiving notification that the update button has been clicked.
     */
    void updateButtonClicked();


    /**
     * Slot for receiving notification when the Magnatune xml file has been downloaded.
     * Triggers a parse of the file to get the info added to the database
     * @param downLoadJob The calling download Job
     */
    void listDownloadComplete( KJob* downLoadJob );


    /**
     * Slot called when the parsing of the Magnatune xml file is completed.
     * Triggers an update of the list view and the genre combo box
     */
    void doneParsing();

    /**
     * Starts the process of redownloading a previously bought album
     */
    void processRedownload();

    /**
     * Slot for receiving notifications of completed download operations
     * @param success Was the operation a success?
     */
    void downloadCompleted( bool success );


    /**
     * Adds all tracks with a common mood to the playlist
     * @param mood The mood of the tracks to add
     */
    void addMoodyTracksToPlaylist( const QString &mood, int count );


     /**
     * Checks if download button should be enabled
     * @param selectedItem the new selected item
     */
    void itemSelected( CollectionTreeItem * selectedItem );


    void moodMapReady(const QMap<QString, int> &map );
    void moodyTracksReady( const Meta::TrackList &tracks );

    void timestampDownloadComplete( KJob * job );
    void favoritesResult( KJob* addToFavoritesJob );

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

    void checkForUpdates();

    /**
     * Adds a magnatune preview track to the playlist.
     * @param item The track to add
     */
    //void addTrackToPlaylist ( Meta::MagnatuneTrack  *item );

    void showSignupDialog();

    static MagnatuneStore *s_instance;

    QString m_currentInfoUrl;
    MagnatuneDownloadHandler *m_downloadHandler;
    MagnatuneRedownloadHandler *m_redownloadHandler;

    QPushButton *m_downloadAlbumButton;
    MagnatuneNeedUpdateWidget *m_needUpdateWidget;

    QAction * m_updateAction;

    bool m_downloadInProgress;

    Meta::MagnatuneAlbum * m_currentAlbum;

    KIO::FileCopyJob * m_listDownloadJob;
    KIO::StoredTransferJob* m_updateTimestampDownloadJob;
    KIO::StoredTransferJob* m_favoritesJob;

    Collections::MagnatuneSqlCollection * m_collection;

    QString m_tempFileName;

    bool m_isMember;
    int m_membershipType;
    QString m_username;
    QString m_password;

    int m_streamType;

    qulonglong m_magnatuneTimestamp;
    ServiceSqlRegistry * m_registry;

    MagnatuneInfoParser * m_magnatuneInfoParser;

    QDialog *m_signupInfoWidget;
};


#endif
