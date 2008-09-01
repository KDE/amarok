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

#ifndef AMAROK_LYRICS_ENGINE
#define AMAROK_LYRICS_ENGINE

#include "ContextObserver.h"

#include "context/DataEngine.h"
#include "context/LyricsManager.h"

#include <kio/job.h>

/**
    This class provides Lyrics data for use in Context applets. 

NOTE: The QVariant data is structured like this:
           * the key name is lyrics
           * the data is a QVariantList with title, artist, lyricsurl, lyrics
*/

using namespace Context;

class LyricsEngine : public DataEngine, public ContextObserver, public LyricsObserver
{
    Q_OBJECT

public:
    LyricsEngine( QObject* parent, const QList<QVariant>& args );
    
    QStringList sources() const;
    
    // reimplemented from Context::Observer
    virtual void message( const ContextState& state );
    
    void newLyrics( QStringList& lyrics );
    void newSuggestions( QStringList& suggest );
    void lyricsMessage( QString& message );
    
protected:
    bool sourceRequested( const QString& name );
    
private:
    void update();
    
    // stores is we have been disabled (disconnected)
    bool m_requested;
    
};

K_EXPORT_AMAROK_DATAENGINE( lyrics, LyricsEngine )

#endif
