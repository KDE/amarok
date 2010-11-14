/****************************************************************************************
 * Copyright (c) 2010 Rainer Sigle <rainer.sigle@web.de>                                *
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

#ifndef AMAROK_TABS_ENGINE
#define AMAROK_TABS_ENGINE

#include "TabsInfo.h"

#include "context/DataEngine.h"
#include "NetworkAccessManagerProxy.h"
#include "core/meta/Meta.h"

#include <QVariant>

class KJob;
using namespace Context;

 /**
   *   This engine provides tab-data for the current song
   */
class TabsEngine : public DataEngine
{
    Q_OBJECT
    public:
        /**
         * \brief Constructor
         *
         * Creates a new instance of the TabsEngine
         */
        TabsEngine( QObject* parent, const QList<QVariant>& args );

        /**
         * \brief Destructor
         *
         * Destroys a TabsEngine instance
         */
        virtual ~TabsEngine();

        /**
         * Returns the sources
         */
        QStringList sources() const;

    protected:
        bool sourceRequestEvent( const QString& name );

    private slots:

        /**
         *   parses the HTML-result from UltimateGuitar.com and extracts the tab-information
         *   http://www.ultimate-guitar.com/search.php?view_state=advanced&band_name=red+hot+chili+peppers&song_name=californication&type[]=200&type[]=400&type[]=300&version_la=
         */
        void resultUltimateGuitarSearch( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e );
        void resultUltimateGuitarTab( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e );

        /**
         *   queries fretplay.com and extracts the tab-information
         *   fretplay.com : http://www.fretplay.com/search-tabs?search=SongName
         */
        void resultFretplaySearch( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e );
        void resultFretplayTab( const KUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e );

      /**
       *   This method will send the info to the applet and order them if every jobs are finished
       */
        void resultFinalize();


    private:

        /**
         * available tab sites
         */
        enum Source { UltimateGuitar, FretPlay };

        /**
         * starts a new tab-search
         */
        void requestTab( QString artist, QString title );

        /**
         * The currently playing track
         */
        Meta::TrackPtr m_currentTrack;

        /**
         * Holds artist and title name of the current track
         */
        QString m_titleName;
        QString m_artistName;

        /**
         * Data strucuture which contains all tab-information for
         * the current song. After fetching this data will be send to the applet
         */
        QList < TabsInfo *> m_tabs;

        /**
         * Hashtable containing the currently active jobs
         */
        QHash < const KUrl, Source> m_urls;

        /**
         * Controls whether guitar-tabs will be fetched
         */
        bool m_fetchGuitar;
        /**
         * Controls whether bass-tabs will be fetched
         */
        bool m_fetchBass;

        /**
         * Helper function which returns the intermediate string between two strings
         */
        QString subStringBetween(const QString src, const QString from, const QString to, bool lastIndexForFrom = false );

        /**
         * returns a list of possible search criteria for the current artist
         */
        QStringList defineArtistSearchCriteria( QString artist );

        /**
         * returns a list of possible search criteria for the current title
         */
        QStringList defineTitleSearchCriteria( QString title );

    private slots:
        /**
         * Prepare the calling of the requestTab method.
         * Launched when the track played on amarok has changed.
         */
        void update();
};

Q_DECLARE_METATYPE ( TabsInfo * )
K_EXPORT_AMAROK_DATAENGINE( tabs, TabsEngine )

#endif
