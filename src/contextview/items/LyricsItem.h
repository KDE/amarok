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

#ifndef LYRICS_ITEM_H
#define LYRICS_ITEM_H

#include "ContextItem.h"
#include "../ContextObserver.h"
#include "../GenericInfoBox.h"

#include <QObject>

using namespace Context;

class LyricsItem : public ContextItem, public ContextObserver
{
    
    Q_OBJECT
    static LyricsItem *s_instance;
    
public:
    
    static LyricsItem *instance()
    {
        if( !s_instance )
            return new LyricsItem();
        return s_instance;
    }
    
    
    void showLyrics( const QString& url );
    
    void message( const QString& message );
    const QString name() { return "lyrics"; }
    const QString shownDuring() { return "context"; }
    void enable() { m_enabled = true; }
    void disable() { m_enabled = false; }
public slots:
    void lyricsResult( QByteArray cXmlDoc = 0, bool cached = false );
    
private:
    
    LyricsItem();
    
    GenericInfoBox *m_lyricsBox;
    
    bool            m_lyricsVisible;
    QString         m_HTMLSource;
    
    bool            m_enabled;
};

#endif
