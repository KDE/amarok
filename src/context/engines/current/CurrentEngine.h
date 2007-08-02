/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@gmail.com>        *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef AMAROK_CURRENT_ENGINE
#define AMAROK_CURRENT_ENGINE

#include "ContextObserver.h"

#include "context/DataEngine.h"

/**
    This class provides context information on the currently playing track. This includes info such as the artist, trackname, album of the current song, etc.

    There is no data source: if you connect to the engine, you immediately
    start getting updates when there is data. 

    The key of the data is "current".
    The data is structured as a QVariantList, with the order:
        * Artist
        * Track
        * Album
        * Rating (0-10)
        * Score
        * Track Length
        * Last Played
        * Number of times played
        * Album cover url (local)

*/

class CurrentEngine : public Context::DataEngine, public ContextObserver
{
    Q_OBJECT
    
public:

    CurrentEngine( QObject* parent, const QStringList& args );
    
    QStringList sources() const;
    void message( const Context::ContextState& state );
    
protected:
    bool sourceRequested( const QString& name );
    
private:
    void update();
    
    bool m_requested;
};

K_EXPORT_AMAROK_DATAENGINE( current, CurrentEngine )

#endif
