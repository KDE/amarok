/****************************************************************************************
 * Copyright (c) 2008 Jeff Mitchell <mitchell@kde.org>                                  *
 * Copyright (c) 2007-2008 Leo Franchi <lfranchi@gmail.com>                             *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_SONGKICK_ENGINE
#define AMAROK_SONGKICK_ENGINE

#include "ContextObserver.h"
#include "context/DataEngine.h"
#include "meta/Meta.h"

#include <kio/job.h>

/**
    This class provides Songkick data for use in Context applets. 
    It provides concert/tour information.
*/

using namespace Context;

class SongkickEngine : public DataEngine, public ContextObserver, Meta::Observer
{
    Q_OBJECT

public:
    SongkickEngine( QObject* parent, const QList<QVariant>& args );
    
    QStringList sources() const;
    
    //reimplemented from ContextObserver
    virtual void message( const ContextState& state );
    
    //reimplemented from Meta::Observer
    using Observer::metadataChanged;
    void metadataChanged( Meta::TrackPtr track );

protected:
    bool sourceRequestEvent( const QString& name );
    
private slots:
    void datesResult( KJob* );
    void ontourResult( KJob* );

private:
    void update();
    
    KJob* m_datesJob;
    KJob* m_ontourJob;
    
    QStringList m_sources;

    Meta::TrackPtr m_currentTrack;
    
    bool m_ontour;
    bool m_dates;
    
};

K_EXPORT_AMAROK_DATAENGINE( songkick, SongkickEngine )

#endif
