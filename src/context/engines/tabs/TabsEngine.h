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
#include "ContextObserver.h"
#include "NetworkAccessManagerProxy.h"
#include "core/meta/Meta.h"

#include <QVariant>

class KJob;
using namespace Context;

 /**
   *   This class provides tabs infromation for usage in context applets
   *   @author Rainer Sigle
   *   @version 0.1
   */
class TabsEngine : public DataEngine, public ContextObserver, public Meta::Observer
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

        /**
         * Overriden from Context::Observer
         */
        virtual void message( const ContextState& state );

        /**
         * This method is called when the metadata of a track has changed.
         * The called class may not cache the pointer
         */
        using Observer::metadataChanged;
        void metadataChanged( Meta::TrackPtr track );

    protected:
        /**
         * When a source that does not currently exist is requested by the
         * consumer, this method is called to give the DataEngine the
         * opportunity to create one.
         *
         * The name of the data source (e.g. the source parameter passed into
         * setData) must be the same as the name passed to sourceRequestEvent
         * otherwise the requesting visualization may not receive notice of a
         * data update.
         *
         * If the source can not be populated with data immediately (e.g. due to
         * an asynchronous data acquisition method such as an HTTP request)
         * the source must still be created, even if it is empty. This can
         * be accomplished in these cases with the follow line:
         *
         *      setData(name, DataEngine::Data());
         *
         * \param source : the name of the source that has been requested
         * \return true if a DataContainer was set up, false otherwise
         */
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
         * starts a new tab-search
         */
        void update( QString artist = "", QString title = "" );

        /**
         * list of our information sources for the tabs
         */
        QStringList m_sources;
        enum Source { UltimateGuitar, FretPlay };

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
         * True if a request has been done, otherwise false
         */
        bool m_requested;

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
         * Controls wether guitar-tabs will be fetched
         */
        bool m_fetchGuitar;
        /**
         * Controls wether bass-tabs will be fetched
         */
        bool m_fetchBass;

        /**
         * Helper function which returns the intermediate string between two strings
         */
        QString subStringBetween(const QString src, const QString from, const QString to, bool lastIndexForFrom = false );
};

Q_DECLARE_METATYPE ( TabsInfo * )
K_EXPORT_AMAROK_DATAENGINE( tabs, TabsEngine )

#endif
