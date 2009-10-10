/****************************************************************************************
 * Copyright (c) 2007 Leo Franchi <lfranchi@kde.org>                                    *
 * Copyright (c) 2009 Seb Ruiz <ruiz@kde.org>                                           *
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

#include "LyricsManager.h"

#include "Debug.h"
#include "EngineController.h"

#include <QDomDocument>

////////////////////////////////////////////////////////////////
//// CLASS LyricsObserver
///////////////////////////////////////////////////////////////

LyricsObserver::LyricsObserver()
    : m_subject( 0 )
{}

LyricsObserver::LyricsObserver( LyricsSubject *s )
    : m_subject( s )
{
    m_subject->attach( this );
}

LyricsObserver::~LyricsObserver()
{
    if( m_subject )
        m_subject->detach( this );
}

////////////////////////////////////////////////////////////////
//// CLASS LyricsSubject
///////////////////////////////////////////////////////////////

void LyricsSubject::sendNewLyrics( QStringList lyrics )
{
    foreach( LyricsObserver* obs, m_observers )
    {
        obs->newLyrics( lyrics );
    }
}

void LyricsSubject::sendNewLyricsHtml( QString lyrics )
{
    foreach( LyricsObserver* obs, m_observers )
    {
        obs->newLyricsHtml( lyrics );
    }
}

void LyricsSubject::sendNewSuggestions( QStringList sug )
{
    foreach( LyricsObserver* obs, m_observers )
    {
        obs->newSuggestions( sug );
    }
}

void LyricsSubject::sendLyricsMessage( QString key, QString val )
{
    DEBUG_BLOCK
    foreach( LyricsObserver* obs, m_observers )
    {
        obs->lyricsMessage( key, val );
    }
}

void LyricsSubject::attach( LyricsObserver *obs )
{
    if( !obs || m_observers.indexOf( obs ) != -1 )
        return;
    m_observers.append( obs );
}

void LyricsSubject::detach( LyricsObserver *obs )
{
    int index = m_observers.indexOf( obs );
    if( index != -1 ) m_observers.removeAt( index );
}

////////////////////////////////////////////////////////////////
//// CLASS LyricsManager
///////////////////////////////////////////////////////////////

LyricsManager* LyricsManager::s_self = 0;

void
LyricsManager::lyricsResult( const QString& lyricsXML, bool cached ) //SLOT
{
    DEBUG_BLOCK
    Q_UNUSED( cached );

    QDomDocument doc;
    if( !doc.setContent( lyricsXML ) )
    {
        debug() << "could not read the xml of lyrics, malformed";
        lyricsError( i18n("Lyrics data could not be parsed") );
        // TODO: how about showing cached lyrics then?
        return;
    }

    QString lyrics;

    QDomElement el = doc.documentElement();

    if ( el.tagName() == "suggestions" )
    {
        const QDomNodeList l = doc.elementsByTagName( "suggestion" );

        debug() << "got suggestion list of length" << l.length();
        if( l.length() ==0 )
        {
            sendLyricsMessage( "notfound", "notfound" );
        }
        else
        {
            QStringList suggested;
            for( uint i = 0; i < l.length(); ++i )
            {
                const QString url    = l.item( i ).toElement().attribute( "url" );
                const QString artist = l.item( i ).toElement().attribute( "artist" );
                const QString title  = l.item( i ).toElement().attribute( "title" );

                suggested << QString( "%1 - %2 - %3" ).arg( title, artist, url );
            }
            // setData( "lyrics", "suggested", suggested );
            // TODO for now suggested is disabled
            sendNewSuggestions( suggested );
        }
    }
    else
    {
        if( !The::engineController()->currentTrack() )
            return;

        lyrics = el.text();

        // FIXME: lyrics != "Not found" will not work when the lyrics script displays i18n'ed
        // error messages
        if ( lyrics != "Not found" )
        {
            // overwrite cached lyrics (as either there were no lyircs available previously OR
            // the user exlicitly agreed to overwrite the lyrics)
            debug() << "setting cached lyrics...";
            The::engineController()->currentTrack()->setCachedLyrics( lyrics ); // TODO: setLyricsByPath?
        }
        else if( !The::engineController()->currentTrack()->cachedLyrics().isEmpty() &&
                  The::engineController()->currentTrack()->cachedLyrics() != "Not found" )
        {
            // we found nothing, so if we have cached lyrics, use it!
            debug() << "using cached lyrics...";
            lyrics = The::engineController()->currentTrack()->cachedLyrics();

            // check if the lyrics data contains "<html" (note the missing closing bracket,
            // this enables XHTML lyrics to be recognized)
            if( lyrics.contains( "<html" , Qt::CaseInsensitive) )
            {
                //we have stored html lyrics, so use that directly
                sendNewLyricsHtml( lyrics );
                return;
            }
        }

        // TODO: why don't we use currentTrack->prettyName() here?
        const QString title = el.attribute( "title" );

        QStringList lyricsData;
        lyricsData << title
            << The::engineController()->currentTrack()->artist()->name()
            << QString() // TODO lyrics site
            << lyrics;

        sendNewLyrics( lyricsData );
    }
}

void
LyricsManager::lyricsResultHtml( const QString& lyricsHTML, bool cached )
{
    DEBUG_BLOCK
    Q_UNUSED( cached )

    // we don't need to deal with suggestions here, because
    // we assume the script has called showLyrics if they could
    // be suggestions. this is for HTML display only

    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    if( currentTrack &&
        !lyricsHTML.isEmpty() )
    {
        sendNewLyricsHtml( lyricsHTML );

        // overwrite cached lyrics (as either there were no lyircs available previously OR
        // the user exlicitly agreed to overwrite the lyrics)
        currentTrack->setCachedLyrics( lyricsHTML );
    }
}

void
LyricsManager::lyricsError( const QString &error )
{
    //if we have cached lyrics there is absolutely no point in not showing these..
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    if( currentTrack && !currentTrack->cachedLyrics().isEmpty() )
    {
        // TODO: add some sort of feedback that we could not fetch new ones
        // so we are showing a cached result
        debug() << "showing cached lyrics!";

        // check if the lyrics data contains "<html" (note the missing closing bracket,
        // this enables XHTML lyrics to be recognized)
        if( currentTrack->cachedLyrics().contains( "<html" , Qt::CaseInsensitive ) )
        {
            //we have stored html lyrics, so use that directly
            sendNewLyricsHtml( currentTrack->cachedLyrics() );
            return;
        }
        else
        {
            const QString title = currentTrack->prettyName();

            QStringList lyricsData;
            lyricsData << title
                << currentTrack->artist()->name()
                << QString() // TODO lyrics site
                << currentTrack->cachedLyrics();

            sendNewLyrics( lyricsData );
            return;
        }
    }

    sendLyricsMessage( "error", error );
}

