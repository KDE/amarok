/***************************************************************************
 * copyright            : (C) 2008 Jeff Mitchell <mitchell@kde.org>        *
 *                      : (C) 2007-2008 Leo Franchi <lfranchi@gmail.com>   *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef SONGKICK_ENGINE
#define SONGKICK_ENGINE

#include "ContextObserver.h"
#include "meta/Meta.h"

#include <context/DataEngine.h>

#include <kio/job.h>

/**
    This class provides Songkick data for use in Context applets. 
    It provides concert/tour information.
    
    NOTE: The QVariant event data is structured like this:
           * The source is the type of event list: (friend, sys, user )
           * The key is the string event title
           * the QVariant data is a QVariantList of the event info (including title ), in this order:
                + title, date, location, city, description, link
              all are strings
*/

using namespace Context;

class SongkickEngine : public DataEngine, public ContextObserver, Meta::Observer
{
    Q_OBJECT
public:
    SongkickEngine( QObject* parent, const QList<QVariant>& args );
    ~SongkickEngine();
    
    QStringList sources() const;
    
    //reimplemented from ContextObserver
    void message( const ContextState& state );
    
    //reimplemented from Meta::Observer
    using Observer::metadataChanged;
    void metadataChanged( Meta::TrackPtr track );

protected:

    bool sourceRequested( const QString& name );
    
private slots:
    void datesResult( KJob* );

private:
    void updateData();
    
    QString getCached( QString path );
    
    KJob* m_datesJob;
    
    QStringList m_sources;

    Meta::TrackPtr m_currentTrack;
    
    bool m_ontour;
    bool m_dates;
    
};

K_EXPORT_AMAROK_DATAENGINE( songkick, SongkickEngine )

#endif
