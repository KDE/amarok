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

#include <QStringList>
#include <QUrl>
#include <QVariant>

class LyricsSubject;

struct LyricsData
{
    QString text;
    QString title;
    QString artist;
    QUrl site;

    void clear()
    {
        text.clear();
        title.clear();
        artist.clear();
        site.clear();
    }
};

class AMAROK_EXPORT LyricsObserver
{
    public:
        LyricsObserver();
        explicit LyricsObserver( LyricsSubject* );
        virtual ~LyricsObserver();

        /**
         *  A lyrics script has returned new lyrics.
         */
        virtual void newLyrics( const LyricsData &lyrics ) { Q_UNUSED( lyrics ); }
        /**
         *  A lyrics script has returned a list of suggested URLs for correct lyrics.
         */
        virtual void newSuggestions( const QVariantList &suggestions ) { Q_UNUSED( suggestions ); }
        /**
         *  A lyrics script has returned some generic message that they want to be displayed.
         */
        virtual void lyricsMessage( const QString& key, const QString &val ) { Q_UNUSED( key ); Q_UNUSED( val ); }

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
    
        void sendNewLyrics( const LyricsData &lyrics );
        void sendNewSuggestions( const QVariantList &suggestions );
        void sendLyricsMessage( const QString &key, const QString &val );
    
    private:
        QList<LyricsObserver*> m_observers;
};

class AMAROK_EXPORT LyricsManager : public LyricsSubject
{
    public:
        static LyricsManager* self() 
        { 
            if( !s_self )
                s_self = new LyricsManager();

            return s_self; 
        }

        void lyricsResult( const QString& lyrics, bool cached = false );
        void lyricsResultHtml( const QString& lyrics, bool cached = false );
        void lyricsError( const QString &error );
        void lyricsNotFound( const QString& notfound );

        /**
          * Sets the given lyrics for the track with the given URL.
          *
          * @param trackUrl The URL of the track.
          * @param lyrics The new lyrics.
          */
        void setLyricsForTrack( const QString &trackUrl, const QString &lyrics ) const;

        /**
         * Tests if the given lyrics are empty.
         *
         * @param lyrics The lyrics which will be tested.
         *
         * @return true if the given lyrics are empty, otherwise false.
         */
        bool isEmpty( const QString &lyrics ) const;

    private:
        LyricsManager() : LyricsSubject() { s_self = this; }

        bool showCached();
        
        static LyricsManager* s_self;
};

Q_DECLARE_METATYPE( LyricsData );

#endif
