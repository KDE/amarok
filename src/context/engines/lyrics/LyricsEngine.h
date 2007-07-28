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

#include <context/DataEngine.h>

#include <kio/job.h>

/**
    This class provides Lyrics data for use in Context applets. 

NOTE: The QVariant data is structured like this:
           * the key name is the song title
           * the data is a QString containing the plaintext lyrics
*/

using namespace Context;

class LyricsEngine : public DataEngine, public ContextObserver
{
    Q_OBJECT
public:
    LyricsEngine( QObject* parent, const QStringList& args );
    
    QStringList sources() const;
    
    void message( const ContextState& state );
protected:
    
    bool sourceRequested( const QString& name );
    
private slots:
    void lyricsResult( QByteArray cXmlDoc = 0, bool cached = false );
    
private:
    void update();
    
    // stores is we have been disabled (disconnected)
    bool m_requested;
    
};

K_EXPORT_AMAROK_DATAENGINE( lyrics, LyricsEngine )

#endif
