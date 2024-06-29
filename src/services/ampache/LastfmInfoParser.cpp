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

#include "core/meta/Meta.h"
#include "core/support/Amarok.h"
#include "core/support/Debug.h"

#include <KLocalizedString>

#include <QNetworkReply>

#include <XmlQuery.h>

void LastfmInfoParser::getInfo(const Meta::TrackPtr &track)
{
    DEBUG_BLOCK
    QMap<QString, QString> query;
    query[ QStringLiteral("method") ] = QStringLiteral("track.getInfo");
    query[ QStringLiteral("track")  ] = track->name();
    query[ QStringLiteral("album")  ] = track->album() ? track->album()->name() : QString();
    query[ QStringLiteral("artist") ] = track->artist() ? track->artist()->name() : QString();
    query[ QStringLiteral("apikey") ] = QLatin1String( Amarok::lastfmApiKey() );

    m_jobs[ QStringLiteral("getTrackInfo") ] = lastfm::ws::post( query );

    connect( m_jobs[ QStringLiteral("getTrackInfo") ], &QNetworkReply::finished, this, &LastfmInfoParser::onGetTrackInfo );
}

void LastfmInfoParser::onGetTrackInfo()
{
    DEBUG_BLOCK
    if( !m_jobs[ QStringLiteral("getTrackInfo") ] )
    {
        debug() << "WARNING: GOT RESULT but no object";
        return;
    }

    switch ( m_jobs[ QStringLiteral("getTrackInfo") ]->error() )
    {
        case QNetworkReply::NoError:
        {
            lastfm::XmlQuery lfm;
            lfm.parse( m_jobs[ QStringLiteral("getTrackInfo") ]->readAll() );
            lastfm::XmlQuery wiki = lfm[QStringLiteral("track")][QStringLiteral("wiki")];
            const QString contentText = wiki[QStringLiteral("content")].text();
            const QString publishedDate = wiki[QStringLiteral("published")].text();

            QString html;
            if( !contentText.isEmpty() )
                html = QStringLiteral("<p><font size=3><i>%2<i></font></p> <p align='right'><font size=1>%1</font></p>").arg( i18n("Updated: %1", publishedDate), contentText );
            else
                html = i18n( "<p>No information found for this track.</p>" );
            Q_EMIT info( html );
            break;
        }
        default:
            break;
    }
    m_jobs[QStringLiteral("getTrackInfo")]->deleteLater();
    m_jobs[QStringLiteral("getTrackInfo")] = nullptr;
}

void LastfmInfoParser::getInfo(const Meta::AlbumPtr &album)
{
    DEBUG_BLOCK
    QMap<QString, QString> query;
    query[ QStringLiteral("method") ] = QStringLiteral("album.getInfo");
    query[ QStringLiteral("album")  ] = album->name();
    query[ QStringLiteral("artist") ] = album->albumArtist() ? album->albumArtist()->name() : QString();
    query[ QStringLiteral("apikey") ] = QLatin1String( Amarok::lastfmApiKey() );

    m_jobs[ QStringLiteral("getAlbumInfo") ] = lastfm::ws::post( query );

    connect( m_jobs[ QStringLiteral("getAlbumInfo") ], &QNetworkReply::finished, this, &LastfmInfoParser::onGetAlbumInfo );
}


void LastfmInfoParser::onGetAlbumInfo()
{
    DEBUG_BLOCK
    if( !m_jobs[ QStringLiteral("getAlbumInfo") ] )
    {
        debug() << "WARNING: GOT RESULT but no object";
        return;
    }

    switch ( m_jobs[ QStringLiteral("getAlbumInfo") ]->error() )
    {
        case QNetworkReply::NoError:
        {
            lastfm::XmlQuery lfm;
            lfm.parse( m_jobs[ QStringLiteral("getAlbumInfo") ]->readAll() );
            lastfm::XmlQuery wiki = lfm[QStringLiteral("album")][QStringLiteral("wiki")];
            const QString summaryText = wiki[QStringLiteral("summary")].text();
            const QString contentText = wiki[QStringLiteral("content")].text();
            const QString publishedDate = wiki[QStringLiteral("published")].text();

            const QString albumUrl = lfm[QStringLiteral("image size=large")].text();

            QString html;
            if( !contentText.isEmpty() )
                html = QString(QStringLiteral("<div align='center'><img src=%1></div><div align='center'><p><font size=3><i>%2<i></font></p> <p align='right'><font size=1>Updated: %3</font></p></div>")).arg( albumUrl, contentText, publishedDate );
            else
                html = i18n( "<p>No information found for this album.</p>" );
            Q_EMIT info( html );
            break;
        }
        default:
            break;
    }
    m_jobs[QStringLiteral("getAlbumInfo")]->deleteLater();
    m_jobs[QStringLiteral("getAlbumInfo")] = nullptr;
}


void LastfmInfoParser::getInfo(const Meta::ArtistPtr &artist)
{
    QMap<QString, QString> query;
    query[ QStringLiteral("method") ] = QStringLiteral("artist.getInfo");
    query[ QStringLiteral("artist") ] = artist->name();
    debug() << "api key is: " << Amarok::lastfmApiKey();
    query[ QStringLiteral("apikey") ] = QLatin1String( Amarok::lastfmApiKey() );

    m_jobs[ QStringLiteral("getArtistInfo") ] = lastfm::ws::post( query );

    connect( m_jobs[ QStringLiteral("getArtistInfo") ], &QNetworkReply::finished, this, &LastfmInfoParser::onGetArtistInfo );

}


void LastfmInfoParser::onGetArtistInfo()
{
    DEBUG_BLOCK
    if( !m_jobs[ QStringLiteral("getArtistInfo") ] )
    {
        debug() << "WARNING: GOT RESULT but no object";
        return;
    }

    switch ( m_jobs[ QStringLiteral("getArtistInfo") ]->error() )
    {
        case QNetworkReply::NoError:
        {
            lastfm::XmlQuery lfm;
            lfm.parse( m_jobs[ QStringLiteral("getArtistInfo") ]->readAll() );
            debug() << lfm.text();
            lastfm::XmlQuery bio = lfm[QStringLiteral("artist")][QStringLiteral("bio")];
            const QString summaryText = bio[QStringLiteral("summary")].text();
            const QString contentText = bio[QStringLiteral("content")].text();
            const QString publishedDate = bio[QStringLiteral("published")].text();

            const QString imageUrl = lfm[QStringLiteral("image size=large")].text();

            QString html;
            if( !contentText.isEmpty() )
                html = QString(QStringLiteral("<div align='left'><img src=%1></div><div align='center'><p><font size=3><i>%2<i></font></p> <p align='right'><font size=1>Updated: %3</font></p></div>")).arg( imageUrl, contentText, publishedDate );
            else
                html = i18n( "<p>No information found for this artist.</p>" );
            Q_EMIT info( html );

            break;
        }
        default:
            break;
    }
    m_jobs[QStringLiteral("getArtistInfo")]->deleteLater();
    m_jobs[QStringLiteral("getArtistInfo")] = nullptr;
}

