/****************************************************************************************
 * Copyright (c) 2012 Sven Krohlas <sven@asbest-online.de>                              *
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

#include "AmazonInfoParser.h"

#include "Amazon.h"
#include "AmazonConfig.h"

#include "core/interfaces/Logger.h"
#include "core/support/Components.h"

#include <QDomDocument>
#include <QTemporaryFile>

#include <KStandardDirs>

void
AmazonInfoParser::getInfo( Meta::ArtistPtr artist )
{
    // atm this should not be called from anywhere
    Q_UNUSED( artist );
}

void
AmazonInfoParser::getInfo( Meta::AlbumPtr album )
{
    showLoading( i18n( "Loading album info..." ) );

    Meta::AmazonAlbum *amazonAlbum = dynamic_cast<Meta::AmazonAlbum *>( album.data() );

    if( !amazonAlbum )
        return;

    QString urlString;
    urlString += MP3_MUSIC_STORE_HOST;
    urlString += "/?apikey=";
    urlString += MP3_MUSIC_STORE_KEY;
    urlString += "&Player=amarok&Location=";
    urlString += AmazonConfig::instance()->country();
    urlString += "&method=LoadAlbum";
    urlString += "&ASIN=" + amazonAlbum->asin();

    QTemporaryFile tempFile;
    tempFile.setAutoRemove( false ); // file must be removed later

    if( !tempFile.open() )
    {
        Amarok::Components::logger()->longMessage( i18n( "<b>MP3 Music Store</b><br/><br/>Error: Unable to write temporary file. :-(" ) );
        return;
    }

    KIO::FileCopyJob *requestJob = KIO::file_copy( urlString, QUrl( tempFile.fileName() ), 0700 , KIO::HideProgressInfo | KIO::Overwrite );

    connect( requestJob, SIGNAL(result(KJob*)), this, SLOT(albumInfoDownloadComplete(KJob*)) );
    requestJob->start();
}

void
AmazonInfoParser::getInfo( Meta::TrackPtr track )
{
    // atm this should not be called from anywhere
    Q_UNUSED( track );
}

void
AmazonInfoParser::showFrontPage()
{
    // TODO: show some stuff here. recommendations? charts?
    QString contextHtml;
    emit( info( contextHtml ) );
}

void
AmazonInfoParser::albumInfoDownloadComplete( KJob *requestJob )
{
    if( requestJob->error() )
    {
        Amarok::Components::logger()->longMessage( i18n( "<b>MP3 Music Store</b><br/><br/>Error: Querying MP3 Music Store database failed. :-(" ) );
        debug() << requestJob->errorString();
        requestJob->deleteLater();
        return;
    }

    QString tempFileName;
    KIO::FileCopyJob *job = dynamic_cast<KIO::FileCopyJob*>( requestJob );

    if( job )
        tempFileName = job->destUrl().toLocalFile();

    QString contextHtml;
    QDomDocument responseDocument;

    QFile responseFile( tempFileName );
    if( !responseFile.open( QIODevice::ReadOnly ) )
    {
        debug() << "ERROR opening temp file";
        QFile::remove( tempFileName );
        return;
    }

    QString errorMsg;
    int errorLine;
    int errorColumn;

    if( !responseDocument.setContent( &responseFile, false, &errorMsg, &errorLine, &errorColumn ) ) // parse error
    {
        debug() << responseDocument.toString();
        debug() << "Parse ERROR";
        debug() << "Message:" << errorMsg;
        debug() << "Line:" << errorLine;
        debug() << "Column:" << errorColumn;
        // let's keep the temp file in case of an error
        //QFile::remove( m_tempFileName );
        return;
    }

    QString trackAsin, albumAsin, albumPrice;
    QString artist, albumTitle, trackTitle;
    QString trackPrice, imgUrl;
    QString addToCartUrl, searchUrl;
    QUrl playableUrl;
    QString cartIcon = QUrl( KStandardDirs::locate( "data", "amarok/icons/hicolor/16x16/actions/amarok_cart_add.png" ) ).toString();

    QDomNodeList albumItemsList = responseDocument.documentElement().firstChildElement( QLatin1String( "albums" ) ).elementsByTagName( QString( "item" ) );
    QDomNodeList trackItemsList = responseDocument.documentElement().firstChildElement( QLatin1String( "tracks" ) ).elementsByTagName( QString( "item" ) );

    // album parsing, there should only be exactly one album
    if( albumItemsList.size() != 1 )
        return;

    albumAsin   = albumItemsList.at( 0 ).firstChildElement( QLatin1String( "asin" ) ).firstChild().nodeValue();
    artist      = albumItemsList.at( 0 ).firstChildElement( QLatin1String( "artist" ) ).firstChild().nodeValue();
    albumTitle  = albumItemsList.at( 0 ).firstChildElement( QLatin1String( "album" ) ).firstChild().nodeValue();
    albumPrice  = albumItemsList.at( 0 ).firstChildElement( QLatin1String( "price" ) ).firstChild().nodeValue();
    imgUrl      = albumItemsList.at( 0 ).firstChildElement( QLatin1String( "img" ) ).firstChild().nodeValue();

    addToCartUrl = "amarok://service-amazonstore?asin=" + albumAsin + "&command=addToCart&name=" + artist + " - " + albumTitle + "&price=" + albumPrice;

    contextHtml = "<html><head><meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"></head><body>";
    contextHtml += "<body style=\"font-family:Verdana, Arial, utopia, sans-serif; font-size:0.8em;\">";
    contextHtml += "<h3>" + artist + " - " + albumTitle + " (" + Amazon::prettyPrice( albumPrice ) + ")</h3>";
    contextHtml += "<a href=\"" + addToCartUrl +"\"><img width=\"200px\" src=\"" + imgUrl + "\"></img><br/><img src=\"file://" + cartIcon + "\" alt =\"" + i18n( "Add album to cart" ) + "\"</img> " + i18n( "Add album to cart" ) + "</a>";
    contextHtml += "<br/><br/><table style=\"font-family:Verdana, Arial, utopia, sans-serif; font-size:0.8em;\"><tr><td></td>";
    contextHtml += "<td><strong>" + i18n( "Artist" ) + "</strong></td>";
    contextHtml += "<td><strong>" + i18n( "Track" ) + "</strong></td></tr>";

    // track parsing
    for( int i = 0; i < trackItemsList.size(); i++ )
    {
        artist      = trackItemsList.at( i ).firstChildElement( QLatin1String( "artist" ) ).firstChild().nodeValue();
        trackPrice  = trackItemsList.at( i ).firstChildElement( QLatin1String( "price" ) ).firstChild().nodeValue();
        trackTitle   = trackItemsList.at( i ).firstChildElement( QLatin1String( "title" ) ).firstChild().nodeValue();
        trackAsin   = trackItemsList.at( i ).firstChildElement( QLatin1String( "asin" ) ).firstChild().nodeValue();

        searchUrl = "amarok://navigate/MP3%20Music%20Store/?filter=" + artist;
        playableUrl = "http://www.amazon." + AmazonConfig::instance()->country() + "/gp/dmusic/get_sample_url.html?ASIN=" + trackAsin;
        addToCartUrl = "amarok://service-amazonstore?asin=" + trackAsin + "&command=addToCart&name=" + artist + " - " + trackTitle + "&price=" + trackPrice;

        contextHtml += "<tr><td><a href=\"" + addToCartUrl + "\"><img src=\"file://" + cartIcon + "\" alt =\"" + i18n( "Add to cart" ) + "\"</img></a></td>";
        contextHtml += "<td><a href=\"" + searchUrl + "\">" + artist + "</a></td>";
        contextHtml += "<td><a href=\"amarok://play/" + playableUrl.toEncoded().toBase64() + "\">" + trackTitle + "</a> (" + Amazon::prettyPrice( trackPrice ) + ")</td></tr>";
    }

    contextHtml += "</table></body></html>";

    QFile::remove( tempFileName );
    responseFile.close();

    emit( info( contextHtml ) );
}

