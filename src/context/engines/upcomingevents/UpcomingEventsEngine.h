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

#include "src/context/ContextObserver.h"
#include <context/DataEngine.h>
#include "src/context/applets/upcomingevents/LastFmEvent.h"
#include "meta/Meta.h"

// Qt
#include <QDomDocument>
#include <QLocale>
#include <QXmlStreamReader>

// KDE
#include <KIO/Job>

using namespace Context;

/**
 * \class UpcomingEventsEngine
 *
 * This class provide UpcomingEvents data for use in Context applets
 */
class UpcomingEventsEngine : public DataEngine, public ContextObserver, Meta::Observer
{
    Q_OBJECT
    Q_PROPERTY( QString selectionType READ selection WRITE setSelection )
        
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

    /**
     * Sets the selection
     *
     * \param selection : the current selection
     */
    void setSelection( const QString& selection );

    /**
     * Returns the current selection
     */
    QString selection();

    /**
     * Returns all the upcoming events
     */
    QList< LastFmEvent > upcomingEvents();
    
    /**
    * Fetches the upcoming events for an artist thanks to the LastFm WebService
    *
    * \param artist_name the name of the artist
    * \return a list of events
    */
    void upcomingEventsRequest(const QString &artist_name);

    /**
     * Parses the upcoming events request result
     */
    void upcomingEventsParseResult(QXmlStreamReader& xml);
    
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
    
private:
    /**
     * Sends the data to the observers (e.g UpcomingEventsApplet)
     */
    void update();

    /**
     * The value can be "AllEvents", "ThisWeek", "ThisMonth" or "ThisYear"
     */
    QString m_timeSpan;

    /**
     * The upcoming events are displayed as web links
     */
    bool m_enabledLinks;

    /**
     * The current track playing
     */
    Meta::TrackPtr m_currentTrack;

    /**
     * The current selection
     */
    QString m_currentSelection;

    /**
     * True if a request has been done, false else
     */
    bool m_requested;

    /**
     * The list of all the sources
     */
    QStringList m_sources;

    /**
     * The list of all upcoming events
     */
    QList< LastFmEvent > m_upcomingEvents;

    /**
     * The XML text, result of LastFm request
     */
    QString m_xml;

    /**
     * The current artist name
     */
    QString m_artistName;

private slots:
    /**
     * Runs the KJob to parse the XML file
     */
    void upcomingEventsResultFetched( KJob* );
};

#endif
