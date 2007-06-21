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
#include "GenericInfoBox.h"
#include "engineobserver.h"

#include <QObject>

using namespace Context;

class LyricsItem : public ContextItem, public EngineObserver
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
    
    void notify( const QString& message );
public slots:
    void lyricsResult( QByteArray cXmlDoc = 0, bool cached = false );
    
protected:
    void engineStateChanged( Engine::State, Engine::State = Engine::Empty );

private:
    
    LyricsItem();
    
    GenericInfoBox *m_lyricsBox;
    
    bool            m_lyricsVisible;
    QString         m_HTMLSource;
};

#endif
