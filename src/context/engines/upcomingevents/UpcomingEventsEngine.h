/****************************************************************************************
 * Copyright (c) 2009 Oleksandr Khayrullin <saniokh@gmail.com>                          *
 * Copyright (c) 2009 Nathan Sala <sala.nathan@gmail.com>                               *
 * Copyright (c) 2009-2010 Ludovic Deveaux <deveaux.ludovic31@gmail.com>                *
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

#ifndef AMAROK_UPCOMINGEVENTS_ENGINE
#define AMAROK_UPCOMINGEVENTS_ENGINE

#include "context/applets/upcomingevents/LastFmEvent.h"
#include "context/DataEngine.h"
#include "core/meta/forward_declarations.h"
#include "network/NetworkAccessManagerProxy.h"

// Qt
#include <QDomDocument>
#include <QLocale>
#include <QXmlStreamReader>

class QNetworkReply;

using namespace Context;

/**
 * \class UpcomingEventsEngine
 *
 * This class provide UpcomingEvents data for use in Context applets
 */
class UpcomingEventsEngine : public DataEngine
{
    Q_OBJECT

public:
    /**
     * \brief Constructor
     *
     * Creates a new instance of UpcomingEventsEngine
     */
    UpcomingEventsEngine( QObject* parent, const QList<QVariant>& args );

    /**
     * \brief Destructor
     *
     * Destroys an UpcomingEventsEngine instance
     */
    virtual ~UpcomingEventsEngine();

protected:
    /**
     * Reimplemented from Plasma::DataEngine
     */
    bool sourceRequestEvent( const QString &name );

private:

    /**
     * filterEvents filters a list of events depending on settings
     * @param events a list of events to filter
     * @return a new list of events that satisfies filter settings
     */
    LastFmEvent::List filterEvents( const LastFmEvent::List &events ) const;

    /**
     * The value can be "AllEvents", "ThisWeek", "ThisMonth" or "ThisYear"
     */
    QString m_timeSpan;

    /**
     * The current artist
     */
    Meta::ArtistPtr m_currentArtist;

    /**
     * Current URLs of events being fetched
     */
    QSet<QUrl> m_urls;

    /**
     * @param ids LastFm's venue ids
     */
    QList<int> m_venueIds;

private Q_SLOTS:
    /**
     * Get events for specific artist
     */
    void updateDataForArtist();

    /**
     * Get events for specific venues
     */
    void updateDataForVenues();

    void artistEventsFetched( const QUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e );
    void venueEventsFetched( const QUrl &url, QByteArray data, NetworkAccessManagerProxy::Error e );
};

#endif
