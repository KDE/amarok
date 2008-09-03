/***************************************************************************
 * copyright            : (C) 2007 Leo Franchi <lfranchi@kde.org>          *
 **************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef LYRICS_MANAGER_H
#define LYRICS_MANAGER_H

#include "amarok_export.h"
#include "Debug.h"

#include <QList>
#include <QByteArray>
#include <QString>

class LyricsSubject;

class AMAROK_EXPORT LyricsObserver
{
public:
    LyricsObserver();
    LyricsObserver( LyricsSubject* );
    virtual ~LyricsObserver();
    
    virtual void newLyrics( QStringList& lyrics ) { Q_UNUSED( lyrics ); }
    virtual void newLyricsHTML( QString& lyrics ) { Q_UNUSED( lyrics ); }
    virtual void newSuggestions( QStringList& suggestions ) { Q_UNUSED( suggestions ); }
    virtual void lyricsMessage( QString& msg ) { Q_UNUSED( msg ); }

private:
    LyricsSubject *m_subject;
};

class LyricsSubject
{
public:
    void attach( LyricsObserver *observer );
    void detach( LyricsObserver *observer );
    
protected:
    LyricsSubject() {}
    virtual ~LyricsSubject() {}
    
    void sendNewLyrics( QStringList lyrics );
    void sendNewLyricsHTML( QString lyrics );
    void sendNewSuggestions( QStringList suggestions );
    void sendLyricsMessage( QString msg );
    
private:
    QList< LyricsObserver* > m_observers;
};

class AMAROK_EXPORT LyricsManager : public LyricsSubject
{
public:
    LyricsManager() : LyricsSubject() { s_self = this; }
    
    static LyricsManager* self() 
    { 
        if( !s_self )
            s_self = new LyricsManager();
    
        return s_self; 
    }
    
    void lyricsResult( const QString& lyrics = 0, bool cached = false );
    void lyricsResultHTML( const QString& lyrics = 0, bool cached = false );
private:
    static LyricsManager* s_self;
};

#endif
