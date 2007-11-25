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
#include "engineobserver.h"
#include "meta/Meta.h" // album observer
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
        * Album cover (QImage)

*/

class CurrentEngine : public Context::DataEngine, 
                      public ContextObserver,
                      public EngineObserver,
                      public Meta::Observer
{
    Q_OBJECT
        
    Q_PROPERTY( int coverWidth READ coverWidth WRITE setCoverWidth  )
    
public:

    CurrentEngine( QObject* parent, const QList<QVariant>& args );
    virtual ~CurrentEngine();
    
    QStringList sources() const;
    void message( const Context::ContextState& state );
    
    int coverWidth() { return m_coverWidth; }
    void setCoverWidth( const int width ) { m_coverWidth = width; }
    
    // reimplemented from Meta::Observer
    void metadataChanged( Meta::Album* album );
    void metadataChanged( Meta::Track *track );

    //reimplemented from EngineObserver
    void engineNewTrackPlaying();
protected:
    bool sourceRequested( const QString& name );
    
private:
    void update();
    
    int m_coverWidth;
    QStringList m_sources;
    bool m_requested;
    Meta::TrackPtr m_currentTrack;
};

K_EXPORT_AMAROK_DATAENGINE( current, CurrentEngine )

#endif
