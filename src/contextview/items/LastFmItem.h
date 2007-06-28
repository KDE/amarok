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

#ifndef LASTFM_ITEM_H
#define LASTFM_ITEM_H  

#include "CloudBox.h"
#include "ContextItem.h"
#include "../ContextObserver.h"
#include "../GenericInfoBox.h"

using namespace Context;

class LastFmItem : public ContextItem, public ContextObserver
{
    Q_OBJECT
        
public:
    LastFmItem();
    
    void message( const QString& msg );
    const QString name() { return "lastfm"; }
    void enable() { m_enabled = true; }
    void disable() { m_enabled = false; }
    
    void showRelatedArtists();
    void showSuggestedSongs();

private:
    
    QString statsHTML(  int score, int rating, bool statsbox = false );
    QString escapeHTMLAttr( const QString &s );
    
    CloudBox* m_relatedArtistsBox;
    GenericInfoBox* m_suggestedSongsBox;
    
    QString m_relHTMLSource;
    QString m_sugHTMLSource;
    bool m_sugBoxVisible;
    bool m_relBoxVisible;
    
    bool m_enabled;
};

#endif
