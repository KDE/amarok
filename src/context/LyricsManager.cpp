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

#define DEBUG_PREFIX "LyricsManager"

#include "LyricsManager.h"

#include "core-impl/collections/support/CollectionManager.h"
#include "core/support/Debug.h"
#include "EngineController.h"

#include <KLocale>

#include <QDomDocument>
#include <QGraphicsTextItem>

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
    DEBUG_BLOCK
    foreach( LyricsObserver* obs, m_observers )
    {
        obs->newLyrics( lyrics );
    }
}

void LyricsSubject::sendNewLyricsHtml( QString lyrics )
{
    DEBUG_BLOCK
    foreach( LyricsObserver* obs, m_observers )
    {
        obs->newLyricsHtml( lyrics );
    }
}

void LyricsSubject::sendNewSuggestions( const QVariantList &sug )
{
    DEBUG_BLOCK
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
            QVariantList suggested;
            for( int i = 0, len = l.length(); i < len; ++i )
            {
                const QString &url    = l.item( i ).toElement().attribute( "url" );
                const QString &artist = l.item( i ).toElement().attribute( "artist" );
                const QString &title  = l.item( i ).toElement().attribute( "title" );
                suggested << ( QStringList() << title << artist << url );
            }
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
        if ( !isEmpty( lyrics ) &&
              lyrics != "Not found" )
        {
            // overwrite cached lyrics (as either there were no lyircs available previously OR
            // the user exlicitly agreed to overwrite the lyrics)
            debug() << "setting cached lyrics...";
            The::engineController()->currentTrack()->setCachedLyrics( lyrics ); // TODO: setLyricsByPath?
        }
        else if( !isEmpty( The::engineController()->currentTrack()->cachedLyrics() ) &&
                  The::engineController()->currentTrack()->cachedLyrics() != "Not found" )
        {
            // we found nothing, so if we have cached lyrics, use it!
            debug() << "using cached lyrics...";
            lyrics = The::engineController()->currentTrack()->cachedLyrics();

            if( isHtmlLyrics( lyrics ) )
            {
                //we have stored html lyrics, so use that directly
                sendNewLyricsHtml( lyrics );
                return;
            }
        }

        // only continue if the given lyrics are not empty
        if ( !isEmpty( lyrics ) )
        {
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
        !isEmpty( lyricsHTML ) )
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
    DEBUG_BLOCK
    if( !showCached() )
    {
        sendLyricsMessage( "error", error );
    }
}


void
LyricsManager::lyricsNotFound( const QString& notfound )
{
    DEBUG_BLOCK
    if( !showCached() )
    {
        //if we have cached lyrics there is absolutely no point in not showing these..
        Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
	if( !currentTrack )
            return;

        Meta::ArtistPtr currentArtist = currentTrack->artist();
	if( !currentArtist )
            return;

        const QString title = currentTrack->prettyName();

        QStringList lyricsData;
        lyricsData << title
        << currentArtist->name()
        << QString() // TODO lyrics site
        << notfound;
        sendNewLyrics( lyricsData );
    }
}


bool LyricsManager::showCached()
{
    DEBUG_BLOCK
    //if we have cached lyrics there is absolutely no point in not showing these..
    Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
    if( currentTrack && !isEmpty( currentTrack->cachedLyrics() ) )
    {
        // TODO: add some sort of feedback that we could not fetch new ones
        // so we are showing a cached result
        debug() << "showing cached lyrics!";

        if( isHtmlLyrics( currentTrack->cachedLyrics() ) )
        {
            //we have stored html lyrics, so use that directly
            sendNewLyricsHtml( currentTrack->cachedLyrics() );
            return true;
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
            return true;
        }
    }
    return false;
}

void LyricsManager::setLyricsForTrack( const QString &trackUrl, const QString &lyrics ) const
{
    DEBUG_BLOCK

    Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( KUrl( trackUrl ) );

    if( track )
        track->setCachedLyrics( lyrics );
    else
        debug() << QString("could not find a track for the given URL (%1) - ignoring.").arg( trackUrl );
}

bool LyricsManager::isHtmlLyrics( const QString &lyrics ) const
{
    // Check if the lyrics data contains "<html" (note the missing closing bracket,
    // this ensures XHTML lyrics are recognized)
    return lyrics.contains( "<html" , Qt::CaseInsensitive );
}

bool LyricsManager::isEmpty( const QString &lyrics ) const
{
    QGraphicsTextItem testItem;

    // Set the text of the TextItem.
    if( isHtmlLyrics( lyrics ) )
        testItem.setHtml( lyrics );
    else
        testItem.setPlainText( lyrics );

    // Get the plaintext content.
    // We use toPlainText() to strip all Html formatting,
    // so we can test if there's any text given.
    QString testText = testItem.toPlainText().trimmed();

    return testText.isEmpty();
}
