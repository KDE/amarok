/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@kde.org>                                    *
 *                                                                                      *
 * This program is free software; you can redistribute it and/or modify it under        *
 * the terms of the GNU General Public License as published by the Free Software        *
 * Foundation; either version 2 of the License, or (at your option) any later           *
 * version.                                                                             *
 *                                                                                      *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY      *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A      *
 * PARTICULAR PURPOSE. See the GNU General Public License for more details.             *
 *                                                                                      *
 * You should have received a copy of the GNU General Public License along with         *
 * this program.  If not, see <http://www.gnu.org/licenses/>.                           *
 ****************************************************************************************/

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
        virtual void newLyricsHtml( QString& lyrics ) { Q_UNUSED( lyrics ); }
        virtual void newSuggestions( QStringList& suggestions ) { Q_UNUSED( suggestions ); }
        virtual void lyricsMessage( QString& key, QString &val ) { Q_UNUSED( key ); Q_UNUSED( val ); }

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
        void sendNewLyricsHtml( QString lyrics );
        void sendNewSuggestions( QStringList suggestions );
        void sendLyricsMessage( QString key, QString val );
    
    private:
        QList<LyricsObserver*> m_observers;
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

        void lyricsResult( const QString& lyrics, bool cached = false );
        void lyricsResultHtml( const QString& lyrics, bool cached = false );
        void lyricsError( const QString &error );

    private:
        static LyricsManager* s_self;
};

#endif
