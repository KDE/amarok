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
#include "core/meta/forward_declarations.h"
#include "NetworkAccessManagerProxy.h"

#include <QVariant>

class KJob;
using namespace Context;

 /**
   *   This engine provides tab-data for the current song
   */
class TabsEngine : public DataEngine
{
    Q_OBJECT
    Q_PROPERTY( QString artistName READ artistName WRITE setArtistName )
    Q_PROPERTY( QString titleName READ titleName WRITE setTitleName )
    Q_PROPERTY( bool fetchGuitarTabs READ fetchGuitar WRITE setFetchGuitar )
    Q_PROPERTY( bool fetchBassTabs READ fetchBass WRITE setFetchBass )

    public:
        TabsEngine( QObject* parent, const QList<QVariant>& args );
        virtual ~TabsEngine();

        QString artistName() const;
        void setArtistName( const QString &artistName );

        QString titleName() const;
        void setTitleName( const QString &titleName );

        bool fetchGuitar() const;
        void setFetchGuitar( const bool fetch );

        bool fetchBass() const;
        void setFetchBass( const bool fetch );

        QStringList sources() const;

    protected:
        bool sourceRequestEvent( const QString &name );

    private slots:
        /**
        *   handling of tab data search results from ultimateguitar.com
        */
        void resultUltimateGuitarSearch( const QUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e );
        void resultUltimateGuitarTab( const QUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e );

        /**
        *   This method will send the info to the applet and order them if every jobs are finished
        */
        void resultFinalize();

        /**
         * Prepare the calling of the requestTab method.
         * Launched when the track played on amarok has changed.
         */
        void update();

    private:
        /**
         * starts a new tab-search
         */
        void requestTab( const QString &artist, const QString &title );

        /**
         *   starts a tab search at ultimateguitar.com
         */
        void queryUltimateGuitar( const QString &artist, const QString &title );

        /**
         * The currently playing track
         */
        Meta::TrackPtr m_currentTrack;

        /**
         * Data strucuture which contains all tab-information for
         * the current song. After fetching this data will be send to the applet
         */
        QList < TabsInfo * > m_tabs;

        /**
         * Set containing urls of active jobs
         */
        QSet < const QUrl > m_urls;

        /**
         * Holds artist and title name of the current track
         */
        QString m_titleName;
        QString m_artistName;

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
        QString subStringBetween( const QString &src, const QString &from, const QString &to,
                                  bool lastIndexForFrom = false );

        /**
         * returns a list of possible search criteria for the current artist
         */
        QStringList defineArtistSearchCriteria( const QString &artist );

        /**
         * returns a list of possible search criteria for the current title
         */
        QStringList defineTitleSearchCriteria( const QString &title );

        /**
         * checks if a tab-fetch job aborted with an error
         * returns true in case of error, false otherwise
         */
        bool netReplyError( NetworkAccessManagerProxy::Error e );
        int m_numAbortedUrls;
};

Q_DECLARE_METATYPE ( TabsInfo * )
AMAROK_EXPORT_DATAENGINE( tabs, TabsEngine )

#endif
