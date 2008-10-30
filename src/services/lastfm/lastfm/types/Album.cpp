/***************************************************************************
 *   Copyright 2005-2008 Last.fm Ltd.                                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.          *
 ***************************************************************************/

#include "Album.h"
#include "Artist.h"
#include "User.h"
#include "../core/CoreUrl.h"
#include "../ws/WsAccessManager.h"
#include "../ws/WsRequestBuilder.h"
#include <QFile>
#include <QTimer>


WsReply*
Album::getInfo() const
{
    return WsRequestBuilder( "album.getInfo" )
            .add( "artist", m_artist )
            .add( "album", m_title )
            .get();
}



WsReply*
Album::getTags() const
{
	return WsRequestBuilder( "album.getTags" )
            .add( "artist", m_artist )
            .add( "album", m_title )
            .get();
}


WsReply*
Album::share( const User& recipient, const QString& message )
{
    return WsRequestBuilder( "album.share" )
		.add( "recipient", recipient )
		.add( "artist", m_artist )
		.add( "album", m_title )
		.addIfNotEmpty( "message", message )
		.post();
}


QUrl
Album::www() const
{
	QString const artist = CoreUrl::encode( m_artist );
	QString const album = CoreUrl::encode( m_title );
	return CoreUrl( "http://www.last.fm/music/" + artist + "/" + album ).localised();	
}


WsReply*
Album::addTags( const QStringList& tags ) const
{
    if (tags.isEmpty())
        return 0;
    
    QString comma_separated_tags;
    foreach( QString const tag, tags)
        comma_separated_tags += tag;
    
    return WsRequestBuilder( "album.addTags" )
            .add( "artist", m_artist )
            .add( "album", m_title )
            .add( "tags", comma_separated_tags )
            .post();
}



/** @author <max@last.fm> 
  */

AlbumImageFetcher::AlbumImageFetcher( const Album& album, Album::ImageSize size )
				 : m_size( (int)size ),
				   m_manager( 0 ),
                   m_nocover( false )
{	
    if (album.isNull()) {
        QTimer::singleShot( 0, this, SLOT(fail()) );
        return;
    }
    
    WsReply* reply = album.getInfo();
	connect( reply, SIGNAL(finished( WsReply* )), SLOT(onGetInfoFinished( WsReply* )) );
}


void
AlbumImageFetcher::onGetInfoFinished( WsReply* reply )
{
	if (!reply->failed())
	    for (; m_size >= 0; --m_size)
        {
            try
            {
                QUrl const url = reply->lfm()["album"]["image size="+size()].text();
                
                // we seem to get a load of album.getInfos where the node exists
                // but the value is ""
                if (!url.isValid())
                    continue;

                m_manager = new WsAccessManager( this );

                QNetworkReply* get = m_manager->get( QNetworkRequest( url ) );
                connect( get, SIGNAL(finished()), SLOT(onImageDataDownloaded()) );
                return;
            }
            catch (CoreDomElement::Exception& e)
            {}
        }

    fail();
}


void
AlbumImageFetcher::onImageDataDownloaded()
{
	QNetworkReply* reply = (QNetworkReply*)sender();
    QByteArray const data = reply->readAll();
    if (data.isEmpty())
        fail();
    else
        emit finished( data );
    
	reply->deleteLater(); //never delete an object in a slot connected to it
						  //always call deleteLater _after_ emit
}


void
AlbumImageFetcher::fail()
{
    m_nocover = true;
    QFile f( ":/lastfm/no/cover.png" );
    f.open( QFile::ReadOnly );
    emit finished( f.readAll() );
}
