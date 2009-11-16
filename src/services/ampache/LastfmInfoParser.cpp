/***************************************************************************************
* Copyright (c) 2009 Dan Meltzer <parallelgrapefruit@gmail.com>                        *
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


#include "LastfmInfoParser.h"

#include "Amarok.h"
#include "Debug.h"

#include <QNetworkReply>
#include <lastfm/XmlQuery>
#include <ws.h>


void LastfmInfoParser::getInfo(Meta::TrackPtr track)
{
    DEBUG_BLOCK
    QMap<QString, QString> query;
    query[ "method" ] = "track.getInfo";
    query[ "track"  ] = track->name();
    query[ "album"  ] = track->album() ? track->album()->name() : QString();
    query[ "artist" ] = track->artist() ? track->artist()->name() : QString();
    query[ "apikey" ] = Amarok::lastfmApiKey();

    m_jobs[ "getTrackInfo" ] = lastfm::ws::post( query );

    connect( m_jobs[ "getTrackInfo" ], SIGNAL( finished() ), SLOT( onGetTrackInfo() ) );
}

void LastfmInfoParser::onGetTrackInfo()
{
    DEBUG_BLOCK
    if( !m_jobs[ "getTrackInfo" ] )
    {
        debug() << "WARNING: GOT RESULT but no object";
        return;
    }

    switch ( m_jobs[ "getTrackInfo" ]->error() )
    {
        case QNetworkReply::NoError:
        {
            lastfm::XmlQuery lfm = m_jobs[ "getTrackInfo" ]->readAll();
            lastfm::XmlQuery wiki = lfm["track"]["wiki"];
            const QString contentText = wiki["content"].text();
            const QString publishedDate = wiki["published"].text();

            QString html;
            if( !contentText.isEmpty() )
                html = QString("<p><font size=3><i>%1<i></font></p> <p align='right'><font size=1>Updated: %2</font></p>").arg( contentText, publishedDate );
            else
                html = i18n( "<p>Sorry, no information Found for this track</p>" );
            emit info( html );
            break;
        }
        default:
            break;
    }
    m_jobs["getTrackInfo"]->deleteLater();
}

void LastfmInfoParser::getInfo(Meta::AlbumPtr album)
{
    DEBUG_BLOCK
    QMap<QString, QString> query;
    query[ "method" ] = "album.getInfo";
    query[ "album"  ] = album->name();
    query[ "artist" ] = album->albumArtist() ? album->albumArtist()->name() : QString();
    query[ "apikey" ] = Amarok::lastfmApiKey();

    m_jobs[ "getAlbumInfo" ] = lastfm::ws::post( query );

    connect( m_jobs[ "getAlbumInfo" ], SIGNAL( finished() ), SLOT( onGetAlbumInfo() ) );
}


void LastfmInfoParser::onGetAlbumInfo()
{
    DEBUG_BLOCK
    if( !m_jobs[ "getAlbumInfo" ] )
    {
        debug() << "WARNING: GOT RESULT but no object";
        return;
    }

    switch ( m_jobs[ "getAlbumInfo" ]->error() )
    {
        case QNetworkReply::NoError:
        {
            lastfm::XmlQuery lfm = m_jobs[ "getAlbumInfo" ]->readAll();
            lastfm::XmlQuery wiki = lfm["album"]["wiki"];
            const QString summaryText = wiki["summary"].text();
            const QString contentText = wiki["content"].text();
            const QString publishedDate = wiki["published"].text();

            QString html;
            if( !contentText.isEmpty() )
                html = QString("<p><font size=3><i>%1<i></font></p> <p align='right'><font size=1>Updated: %2</font></p>").arg( contentText, publishedDate );
            else
                html = i18n( "<p>Sorry, no information Found for this album</p>" );
            emit info( html );
            break;
        }
        default:
            break;
    }
    m_jobs["getAlbumInfo"]->deleteLater();
}


void LastfmInfoParser::getInfo(Meta::ArtistPtr artist)
{
    QMap<QString, QString> query;
    query[ "method" ] = "artist.getInfo";
    query[ "artist" ] = artist->name();
    debug() << "api key is: " << Amarok::lastfmApiKey();
    query[ "apikey" ] = Amarok::lastfmApiKey();

    m_jobs[ "getArtistInfo" ] = lastfm::ws::post( query );

    connect( m_jobs[ "getArtistInfo" ], SIGNAL( finished() ), SLOT( onGetArtistInfo() ) );

}


void LastfmInfoParser::onGetArtistInfo()
{
    DEBUG_BLOCK
    if( !m_jobs[ "getArtistInfo" ] )
    {
        debug() << "WARNING: GOT RESULT but no object";
        return;
    }

    switch ( m_jobs[ "getArtistInfo" ]->error() )
    {
        case QNetworkReply::NoError:
        {
            lastfm::XmlQuery lfm = m_jobs[ "getArtistInfo" ]->readAll();
            debug() << lfm.text();
            lastfm::XmlQuery bio = lfm["artist"]["bio"];
            const QString summaryText = bio["summary"].text();
            const QString contentText = bio["content"].text();
            const QString publishedDate = bio["published"].text();

            QString html;
            if( !contentText.isEmpty() )
                html = QString("<p><font size=3><i>%1<i></font></p> <p align='right'><font size=1>Updated: %2</font></p>").arg( contentText, publishedDate );
            else
                html = i18n( "<p>Sorry, no information Found for this artist</p>" );
            emit info( html );

            break;
        }
        default:
            break;
    }
    m_jobs["getArtistInfo"]->deleteLater();
}

#include "LastfmInfoParser.moc"
