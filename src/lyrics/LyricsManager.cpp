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

#include <QJsonDocument>
#include <QJsonObject>
#include <QTextEdit>
#include <QXmlStreamReader>

#include <KLocalizedString>


#define APIURL QStringLiteral("https://api.lyrics.ovh/v1/")


LyricsManager* LyricsManager::s_self = nullptr;

LyricsManager::LyricsManager()
{
    s_self = this;
    connect( The::engineController(), &EngineController::trackChanged, this, &LyricsManager::newTrack );
}

void
LyricsManager::newTrack( const Meta::TrackPtr &track )
{
    loadLyrics( track );
}

void
LyricsManager::lyricsResult( const QByteArray& lyricsXML, Meta::TrackPtr track ) //SLOT
{
    DEBUG_BLOCK

    QXmlStreamReader xml( lyricsXML );
    while( !xml.atEnd() )
    {
        xml.readNext();

        if( xml.name() == QStringLiteral("lyric") || xml.name() == QStringLiteral( "lyrics" ) )
        {
            QString lyrics( xml.readElementText() );
            if( !isEmpty( lyrics ) )
            {
                // overwrite cached lyrics (as either there were no lyrics available previously OR
                // the user explicitly agreed to overwrite the lyrics)
                debug() << "setting cached lyrics...";
                track->setCachedLyrics( lyrics ); // TODO: setLyricsByPath?
                Q_EMIT newLyrics( track );
            }
            else
            {
                ::error() << i18n("Retrieved lyrics is empty");
                return;
            }
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

            if( !suggestions.isEmpty() )
                Q_EMIT newSuggestions( suggestions );

            return;
        }
    }

    if( xml.hasError() )
    {
        warning() << "errors occurred during reading lyrics xml result:" << xml.errorString();
        Q_EMIT error( i18n("Lyrics data could not be parsed") );
    }
}

void LyricsManager::loadLyrics( Meta::TrackPtr track, bool overwrite )
{
    DEBUG_BLOCK

    if( !track )
    {
        debug() << "no current track";
        return;
    }

    // -- get current title and artist
    QString title = track->name();
    QString artist = track->artist() ? track->artist()->name() : QString();

    sanitizeTitle( title );
    sanitizeArtist( artist );

    if( !isEmpty( track->cachedLyrics() ) && !overwrite )
    {
        debug() << "Lyrics already cached.";
        return;
    }

    QUrl url( APIURL + artist + QLatin1Char('/') + title );
    m_trackMap.insert( url, track );

    connect( NetworkAccessManagerProxy::instance(), &NetworkAccessManagerProxy::requestRedirectedUrl,
             this, &LyricsManager::updateRedirectedUrl);

    NetworkAccessManagerProxy::instance()->getData( url, this, &LyricsManager::lyricsLoaded );
}

void LyricsManager::lyricsLoaded( const QUrl& url, const QByteArray& data, const NetworkAccessManagerProxy::Error &err )
{
    DEBUG_BLOCK

    if( err.code )
    {
        warning() << "A network error occurred:" << err.description;
        return;
    }

    Meta::TrackPtr track = m_trackMap.take( url );
    if( !track )
    {
        warning() << "No track belongs to this url:" << url.url();
        return;
    }

    QJsonDocument document = QJsonDocument::fromJson( data );

    if( document.isNull() || !document.object().contains( QStringLiteral("lyrics") ) )
    {
        if( track->album() && track->album()->albumArtist() )
        {
            QString albumArtist = track->album()->albumArtist()->name();
            QString artist = track->artist() ? track->artist()->name() : QString();
            QString title = track->name();
            sanitizeTitle( title );
            sanitizeArtist( artist );
            sanitizeArtist( albumArtist );

            //Try with album artist
            if( url == QUrl( APIURL + artist + QLatin1Char('/') + title ) && albumArtist != artist )
            {
                debug() << "Try again with album artist.";

                QUrl newUrl( APIURL + albumArtist + QLatin1Char('/') + title );
                m_trackMap.insert( newUrl, track );
                NetworkAccessManagerProxy::instance()->getData( newUrl, this, &LyricsManager::lyricsLoaded );
                return;
            }
        }

        debug() << "No lyrics found for track:" << track->name();
        return;
    }

    QString lyricString = document.object().value( QStringLiteral("lyrics") ).toString();
    if( lyricString.contains( QStringLiteral( "\r\n" ) ) )
    {
        int lindex = lyricString.indexOf( QStringLiteral( "\r\n" ) ); // lyrics.ovh lyrics start from second row
        lyricsResult( ( QStringLiteral( "<lyric>" ) + lyricString.mid( lindex+2 ) + QStringLiteral( "</lyric>" ) ).toUtf8(), track );
    }
    else
        warning() << "No lyrics found in data:" << data;
}

void LyricsManager::sanitizeTitle( QString& title )
{
    const QString magnatunePreviewString = QStringLiteral( "PREVIEW: buy it at www.magnatune.com" );

    if( title.contains(magnatunePreviewString, Qt::CaseSensitive) )
        title = title.remove( QStringLiteral(" (") + magnatunePreviewString + QLatin1Char(')') );

    title = title.remove( QStringLiteral( "(Live)" ) );
    title = title.remove( QStringLiteral( "(live)" ) );
    title = title.replace( QLatin1Char('`'), QStringLiteral( "'" ) );
    title = title.replace( QLatin1Char('&'), QStringLiteral( "%26" ) );
}

void LyricsManager::sanitizeArtist( QString& artist )
{
    const QString magnatunePreviewString = QStringLiteral( "PREVIEW: buy it at www.magnatune.com" );

    if( artist.contains(magnatunePreviewString, Qt::CaseSensitive) )
        artist = artist.remove( QStringLiteral(" (") + magnatunePreviewString + QLatin1Char(')') );

    // strip "featuring <someone else>" from the artist
    int strip = artist.toLower().indexOf( QLatin1String(" ft. "));
    if ( strip != -1 )
        artist = artist.mid( 0, strip );

    strip = artist.toLower().indexOf( QLatin1String(" feat. ") );
    if ( strip != -1 )
        artist = artist.mid( 0, strip );

    strip = artist.toLower().indexOf( QLatin1String(" featuring ") );
    if ( strip != -1 )
        artist = artist.mid( 0, strip );

    artist = artist.replace( QLatin1Char('`'), QStringLiteral( "'" ) );
    artist = artist.replace( QLatin1Char('&'), QStringLiteral( "%26" ) );
}

bool LyricsManager::isEmpty( const QString &lyrics ) const
{
    QTextEdit testItem;

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

void LyricsManager::updateRedirectedUrl(const QUrl& oldUrl, const QUrl& newUrl)
{
    if( m_trackMap.contains( oldUrl ) && !m_trackMap.contains( newUrl ) )
    {
        // Get track for the old URL.
        Meta::TrackPtr track = m_trackMap.value( oldUrl );

        // Replace with redirected url for correct lookup
        m_trackMap.insert( newUrl, track );
        m_trackMap.remove( oldUrl );
    }
}
