/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@gmail.com>                                  *
 * Copyright (c) 2008 Mark Kretschmann <kretschmann@kde.org>                            *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Pulic License for more details.              *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

#ifndef AMAROK_LYRICS_ENGINE
#define AMAROK_LYRICS_ENGINE

#include "ContextObserver.h"
#include "context/DataEngine.h"
#include "context/LyricsManager.h"
#include "meta/Meta.h"

#include <kio/job.h>

/**
    This class provides Lyrics data for use in Context applets. 

NOTE: The QVariant data is structured like this:
           * the key name is lyrics
           * the data is a QVariantList with title, artist, lyricsurl, lyrics
*/

using namespace Context;

class LyricsEngine : public DataEngine, public ContextObserver, public LyricsObserver, public Meta::Observer
{
    Q_OBJECT

public:
    LyricsEngine( QObject* parent, const QList<QVariant>& args );
    
    QStringList sources() const;
    
    // reimplemented from Context::Observer
    virtual void message( const ContextState& state );
    
    // reimplemented from LyricsObserver
    void newLyrics( QStringList& lyrics );
    void newLyricsHtml( QString& lyrics );
    void newSuggestions( QStringList& suggest );
    void lyricsMessage( QString& key, QString& val );

    // reimplemented from Meta::Observer
    using Observer::metadataChanged;
    void metadataChanged( Meta::TrackPtr track );
    
protected:
    bool sourceRequestEvent( const QString& name );
    
private:
    void update();
    
    // stores is we have been disabled (disconnected)
    bool m_requested;
   
    Meta::TrackPtr m_currentTrack;
    // Cache the title/artist of the current track so we can check against
    // metadata updates. We only want to update the lyrics if either the
    // title or the artist change (but not other attributes like rating, score,
    // composer etc).
    QStringList m_currentLyricsList,m_prevLyricsList;
    QStringList m_currentSuggestionsList,m_prevSuggestionsList;
    QString m_currentLyrics,m_prevLyrics;
    QString        m_title;
    QString        m_artist;
};

K_EXPORT_AMAROK_DATAENGINE( lyrics, LyricsEngine )

#endif
