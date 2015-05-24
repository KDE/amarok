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

#include "EngineController.h"
#include "core/meta/Meta.h"
#include "core/support/Debug.h"
#include "core-impl/collections/support/CollectionManager.h"

#include <KLocale>

#include <QGraphicsTextItem>
#include <QXmlStreamReader>
#include <QTextDocument>

////////////////////////////////////////////////////////////////
//// CLASS LyricsObserver
///////////////////////////////////////////////////////////////

LyricsObserver::LyricsObserver()
    : m_subject( 0 )
{
    qRegisterMetaType<LyricsData>("LyricsData");
}

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

void LyricsSubject::sendNewLyrics( const LyricsData &lyrics )
{
    DEBUG_BLOCK
    foreach( LyricsObserver* obs, m_observers )
    {
        obs->newLyrics( lyrics );
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

void LyricsSubject::sendLyricsMessage( const QString &key, const QString &val )
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

    QXmlStreamReader xml( lyricsXML );
    while( xml.readNextStartElement() )
    {
        if( xml.name() == QLatin1String("lyric") )
        {
            Meta::TrackPtr currentTrack = The::engineController()->currentTrack();
            if( !currentTrack )
                return;

            QString lyrics( xml.readElementText() );
            if( !isEmpty( lyrics ) )
            {
                // overwrite cached lyrics (as either there were no lyircs available previously OR
                // the user exlicitly agreed to overwrite the lyrics)
                debug() << "setting cached lyrics...";
                currentTrack->setCachedLyrics( lyrics ); // TODO: setLyricsByPath?
            }
            else if( !isEmpty( currentTrack->cachedLyrics() ) )
            {
                // we found nothing, so if we have cached lyrics, use it!
                debug() << "using cached lyrics...";
                lyrics = currentTrack->cachedLyrics();
            }
            else
            {
                lyricsError( i18n("Retrieved lyrics is empty") );
                return;
            }

            QString artist = currentTrack->artist() ? currentTrack->artist()->name() : QString();
            LyricsData data = { lyrics, currentTrack->name(), artist, QUrl() };
            sendNewLyrics( data );
            return;
        }
        else if( xml.name() == QLatin1String("suggestions") )
        {
            QVariantList suggestions;
            while( xml.readNextStartElement() )
            {
                if( xml.name() != QLatin1String("suggestion") )
                    continue;

                const QXmlStreamAttributes &a = xml.attributes();

                QString artist = a.value( QLatin1String("artist") ).toString();
                QString title = a.value( QLatin1String("title") ).toString();
                QString url = a.value( QLatin1String("url") ).toString();

                if( !url.isEmpty() )
                    suggestions << ( QStringList() << title << artist << url );
                xml.skipCurrentElement();
            }

            debug() << "got" << suggestions.size() << "suggestions";
            if( suggestions.isEmpty() )
                sendLyricsMessage( "notFound", "notfound" );
            else
                sendNewSuggestions( suggestions );
            return;
        }
        xml.skipCurrentElement();
    }

    if( xml.hasError() )
    {
        warning() << "errors occurred during reading lyrics xml result:" << xml.errorString();
        lyricsError( i18n("Lyrics data could not be parsed") );
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
    if( currentTrack && !isEmpty( lyricsHTML ) )
    {
        QString artist = currentTrack->artist() ? currentTrack->artist()->name() : QString();
        LyricsData data = { lyricsHTML, currentTrack->name(), artist, QUrl() };
        sendNewLyrics( data );

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
        sendLyricsMessage( "notfound", notfound );
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

        QString lyrics = currentTrack->cachedLyrics();
        QString artist = currentTrack->artist() ? currentTrack->artist()->name() : QString();
        LyricsData data = { lyrics, currentTrack->name(), artist, QUrl() };
        sendNewLyrics( data );
        return true;
    }
    return false;
}

void LyricsManager::setLyricsForTrack( const QString &trackUrl, const QString &lyrics ) const
{
    DEBUG_BLOCK

    Meta::TrackPtr track = CollectionManager::instance()->trackForUrl( QUrl( trackUrl ) );

    if( track )
        track->setCachedLyrics( lyrics );
    else
        debug() << QString("could not find a track for the given URL (%1) - ignoring.").arg( trackUrl );
}

bool LyricsManager::isEmpty( const QString &lyrics ) const
{
    QGraphicsTextItem testItem;

    // Set the text of the TextItem.
    if( Qt::mightBeRichText( lyrics ) )
        testItem.setHtml( lyrics );
    else
        testItem.setPlainText( lyrics );

    // Get the plaintext content.
    // We use toPlainText() to strip all Html formatting,
    // so we can test if there's any text given.
    QString testText = testItem.toPlainText().trimmed();

    return testText.isEmpty();
}
