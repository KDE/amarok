/****************************************************************************************
 * Copyright (c) 2009 Oleksandr Khayrullin <saniokh@gmail.com>                          *
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

#include "ContextObserver.h"
#include "meta/Meta.h"

#include <context/DataEngine.h>

#include <KIO/Job>
#include <QLocale>


/**
    This class provide UpcomingEvents data for use in Context applets.
*/

using namespace Context;

class UpcomingEventsEngine : public DataEngine, public ContextObserver, Meta::Observer
{
    Q_OBJECT
    Q_PROPERTY( QString selectionType READ selection WRITE setSelection )
        
public:
    UpcomingEventsEngine( QObject* parent, const QList<QVariant>& args );
    virtual ~UpcomingEventsEngine();
    
    QStringList sources() const;
    
    // reimplemented from Context::Observer
    virtual void message( const ContextState& state );

    // reimplemented from Meta::Observer
    using Observer::metadataChanged;
    void metadataChanged( Meta::TrackPtr track );

    void setSelection( const QString& selection ) { m_currentSelection = selection; }
    QString selection() { return m_currentSelection; }
    
protected:
    bool sourceRequestEvent( const QString& name );
    
private:
    void update();

    QString m_timeSpan;
    bool m_enabledLinks;
    
    void reloadUpcomingEvents();
    
    KJob* m_upcomingEventsJob;

    Meta::TrackPtr m_currentTrack;
        
    QString m_currentSelection;
    bool m_requested;
    QStringList m_sources;
    short m_triedRefinedSearch;

};

K_EXPORT_AMAROK_DATAENGINE( upcomingEvents, UpcomingEventsEngine )

#endif

